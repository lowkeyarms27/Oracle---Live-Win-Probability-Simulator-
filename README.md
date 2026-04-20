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

---

## What I Learned Building This

### Monte Carlo Simulation
Before this project I had never run a simulation to estimate probability. The core idea is simple — run the same scenario 1,000 times with randomness, count the outcomes, and divide. What made it hard was *calibrating* those simulations against reality. Raw coin-flip logic gives you 50/50 everywhere. Getting to "Split A-site has a 42% base attacker win rate" required learning how to anchor a model against published tournament statistics and work backwards to find the right coefficients.

### Machine Learning Without a Framework
The calibrator trains with scikit-learn but runs inference completely manually — just matrix math and a sigmoid function. This forced me to actually understand what logistic regression is doing rather than treating it as a black box. I now know what a feature vector is, what standardisation does, what an intercept is, and why you need to save the scaler's mean/std alongside the weights.

### Multi-Agent AI Architecture
I'd used single AI prompts before. Here I learned to chain agents where each one has a specific role and adversarial relationship to the others. The Debater arguing *against* the Simulator's output was the key insight — a single AI confidently saying "70% ATK" is less useful than one AI saying 70% and another immediately asking "but did you account for the defuser?" That tension produces better outputs than either alone.

### Real-Time Data with WebSockets
REST gives you data when you ask for it. WebSockets keep a connection open and push data as it changes. Building the WS endpoint in FastAPI and consuming it in React taught me how state flows in a live system — how to handle reconnection, heartbeats, and the difference between "the connection is open" and "we're actually receiving predictions."

### Feature Engineering
This was the most underrated skill I picked up. Raw game state (5 players alive, 270 HP, spike planted) means nothing to an ML model. Turning it into meaningful numbers — normalised HP delta, zone aggression scores, defuser pressure as a binary flag only active post-plant — is where the domain knowledge actually lives. A better feature set matters more than a more complex model.

### Windows Encoding Edge Cases
I hit a wall where the backend returned 500 errors in production but passed all tests locally. The root cause was Python source files with Unicode em dashes (—) and box-drawing characters in string literals that Windows' cp1252 codec couldn't encode when uvicorn serialised the HTTP response. This taught me that "works on my machine" often means "works with my default locale" — and that keeping source files ASCII-clean is a production concern, not just a style preference.

### Frontend Performance — Code Splitting
The initial bundle was 599 KB because recharts (the charting library) was bundled with everything else. Adding `React.lazy()` and `Suspense` split it into per-route chunks so the charting code only loads when the user visits a page that needs it. The initial load dropped to 225 KB. I learned that bundle size is a concrete, measurable thing — not just a vague "make it fast" concern.

### Dev Proxy Configuration
The Vite dev proxy routes frontend API calls to the backend so CORS isn't an issue in development. I learned that WebSocket upgrades need their own proxy rule (`ws: true`) separate from regular HTTP, and that rule ordering matters — a more specific `/api/ws` rule must come before the broader `/api` rule or WebSocket connections silently fall through to the HTTP handler.

### Probability Thinking as a Product Feature
The hardest thing wasn't technical. It was figuring out what "useful" means for a win probability tool. A number alone isn't useful — people need to know *why* it is what it is and *what could change it*. That's why the Debater exists. Showing 78% ATK with the note "but defenders have the defuser in position — actual closer to 70%" is a product decision as much as a technical one. I learned to think of uncertainty as something to surface, not hide.
