import os

GEMINI_API_KEY   = os.getenv("GEMINI_API_KEY", "")
GEMINI_MODEL     = "gemini-2.0-flash"

# Monte Carlo
MC_ITERATIONS    = 1000
STATE_INTERVAL_S = 5      # seconds between state vector updates

# Resource weights -- how much each resource shifts win probability
# Valorant
VAL_WEIGHTS = {
    "hp_per_point":        0.0012,   # 1 HP = 0.12% per player
    "shield_bonus":        0.03,     # having full shield = +3%
    "smoke":               0.08,     # per smoke available to attacker
    "flash":               0.05,
    "molly":               0.06,
    "recon":               0.10,     # sova dart / fade haunt
    "ultimate_attacker":   0.12,     # ready ult on attacker side
    "ultimate_defender":   0.10,
    "eco_penalty":         0.20,     # full buy vs eco round
    "player_advantage":    0.15,     # per extra player alive (5v4 etc)
    "spike_planted":       0.35,     # spike planted = massive shift
    "time_pressure_def":   0.004,    # per second remaining (favors defenders)
}

# Rainbow Six Siege
R6_WEIGHTS = {
    "hp_per_point":        0.0015,
    "armor_bonus":         0.04,
    "hard_breach":         0.12,
    "smoke_grenade":       0.08,
    "drone":               0.15,
    "barricade":           0.03,
    "gadget_generic":      0.05,
    "player_advantage":    0.18,
    "operator_tier":       0.06,     # elite operator pick bonus
    "time_pressure_atk":  0.005,    # per second remaining (favors attackers)
}

GAME_WEIGHTS = {
    "valorant": VAL_WEIGHTS,
    "r6siege":  R6_WEIGHTS,
}
