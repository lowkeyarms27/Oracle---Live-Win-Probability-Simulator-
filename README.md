# Project Oracle — Live Win-Probability & Simulation Engine

> Real-time esports outcome predictor with Monte Carlo simulation, adversarial AI debater, and broadcast overlay. Built for Valorant and R6 Siege.

---

## What It Does

Most esports viewers — and even casters — don't actually know who is winning a round until it ends. Oracle fixes that.

It ingests **9 live game-state sensors** and runs **1,000 Monte Carlo simulations per second** to produce a calibrated win probability for the attacking side. An adversarial **Debater Agent** then argues *against* the simulation's conclusion, surfacing hidden risks the model can't see (defuser position, psychological momentum, map geometry). The final output blends both: 70% simulation + 30% debater adjustment.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Oracle Backend                        │
│                                                          │
│  Game State (9 sensors)                                  │
│       │                                                  │
│       ▼                                                  │
│  ┌──────────────┐    ┌──────────────┐                   │
│  │  Simulator   │───▶│  Forecaster  │──▶ WebSocket tick │
│  │ (1000x MC)   │    │  (blender)   │                   │
│  └──────────────┘    └──────────────┘                   │
│       │                    ▲                             │
│       ▼                    │                             │
│  ┌──────────────┐          │                             │
│  │   Debater    │──────────┘                             │
│  │ (Adversarial)│                                        │
│  └──────────────┘                                        │
└─────────────────────────────────────────────────────────┘
         │
         ▼ WebSocket / REST
┌─────────────────────────────────────────────────────────┐
│                   React Frontend                         │
│   Live View │ Simulator │ Match History │ OBS Overlay   │
└─────────────────────────────────────────────────────────┘
```

### The 9 Sensors

| # | Sensor | Effect |
|---|--------|--------|
| 1 | ATK players alive | +18% per extra player |
| 2 | DEF players alive | -18% per deficit |
| 3 | ATK total HP | Normalized HP delta |
| 4 | DEF total HP | Normalized HP delta |
| 5 | ATK utility count | Smokes/flashes available |
| 6 | DEF utility count | Counter-utility |
| 7 | Spike planted | +38% for attackers |
| 8 | Positioning delta | Zone aggression score (a-site / mid / ct / spawn) |
| 9 | Defuser pressure | Defenders holding defuser post-plant (-8%) |

Plus context inputs: economy (weapon tier 0-3), time remaining, map/site prior, round history.

---

## The Three Agents

### 1. Simulator
Logistic regression calibrated on 20,000 synthetic rounds matching published Valorant tournament statistics:
- Base attacker win rate: 46%
- 5v4 player advantage: +18%
- Spike planted: +38%
- Full-buy vs eco: +23%
- Defuser in position (post-plant): -8%
- ATK executing on-site: +7%

Runs 1,000 Monte Carlo duel simulations per tick, returns a raw win probability.

### 2. Debater
Adversarial agent (Gemini 2.0 Flash + rule-based fallback) that argues *against* the simulation. It flags:
- Defuser in position ("Team B has the defuser — adds 8% to their real conversion rate")
- Psychological momentum (3-round win streaks)
- Map geometry (Split A narrow chokes, Bind teleporter retakes)
- Economy mismatches (pistol vs rifle execute)
- Positioning (defenders playing forward cuts rotations)
- 1vN clutch variance (Monte Carlo underweights non-linear crossfires)

### 3. Forecaster
Blends simulation and debate: `blended = 0.70 * sim + 0.30 * debate_adjusted`

Maintains a rolling 10-minute probability timeline per match.

---

## Stack

| Layer | Technology |
|-------|------------|
| Backend | FastAPI + WebSockets |
| ML | Logistic regression (sklearn fit, manual inference at runtime) |
| AI | Gemini 2.0 Flash (adversarial debater) |
| Frontend | React + Vite + CSS Modules |
| Charts | Recharts |
| Games | Valorant, R6 Siege |

---

## Features

### Live View
Real-time probability gauge that plays through a mock match tick-by-tick. Connect to a live backend via WebSocket — the "Connect Backend" button opens a real WS connection, sends each tick's state, and swaps in backend predictions in real time.

### Scenario Simulator (What-If Machine)
Build any game state from scratch:
- Set player counts, HP, utility, economy, phase, time
- Choose positioning (ATK Executing / Approaching / Even / DEF Forward)
- Toggle spike planted and DEF has defuser
- Set prior round results (up to 3) to trigger momentum rules
- Run 1,000 Monte Carlo simulations instantly
- Falls back to client-side simulation when backend is offline (shown with "Offline Mode" badge)

### Match History
Round-by-round log with probability bar, round winner, key moment, and duration. Full timeline chart for the current round.

### Broadcast Overlay
Transparent-background widget at `/overlay`, designed for OBS window capture. Shows ATK/DEF probability bars, phase, time, spike badge, and key factor. Auto-connects to the backend WebSocket — live dot turns green when receiving real predictions.

---

## Map-Site Priors (Valorant)

| Map | A-site | B-site | C-site |
|-----|--------|--------|--------|
| Ascent | 44% | 49% | -- |
| Bind | 43% | 47% | -- |
| Haven | 48% | 50% | 46% |
| Split | 42% | 51% | -- |
| Icebox | 45% | 44% | -- |
| Fracture | 47% | 48% | -- |
| Pearl | 46% | 47% | -- |
| Lotus | 47% | 48% | 46% |
| Sunset | 45% | 48% | -- |

---

## Running Locally

### Backend
```bash
cd Oracle/backend
pip install fastapi uvicorn numpy scikit-learn google-genai
# Optional: set GEMINI_API_KEY in environment for AI debater
# Without it, falls back to the rule-based debater automatically
uvicorn backend.main:app --host 0.0.0.0 --port 8002 --reload
```

The calibrator trains automatically on first run (~2s) and caches weights to `data/calibrator_weights.json`.

### Frontend
```bash
cd Oracle/frontend-react
npm install
npm run dev
# Runs on http://localhost:5176
# API proxied to http://localhost:8002 via Vite
```

### OBS Overlay
1. Start the frontend dev server
2. In OBS: Add Source > Browser > `http://localhost:5176/overlay`
3. Set width: 700px, height: 120px
4. Check "Shutdown source when not visible"

---

## Scenario Presets

| Preset | Description |
|--------|-------------|
| 5v5 Even Start | Balanced full-buy opening |
| 3v5 Deficit (ATK) | Attacker numbers disadvantage |
| Post-plant 3v2 | Spike planted, DEF has defuser |
| Eco 3v5 (Split A) | Pistol execute into full-buy on hardest site |
| R6: 2v3 with plant | Siege bomb planted, 2 attackers remaining |
| R6: 5v5 Even Start | Siege balanced opening |
| R6: 1v3 Roamer Clutch | Single attacker clutch scenario |
| R6: Pistol Rush 5v4 | Eco rush with player advantage |
