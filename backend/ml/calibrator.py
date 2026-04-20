"""
Logistic regression calibrator trained on synthetic Valorant/R6 round data.
9 features (sensors):
  1. player_delta       -- ATK alive - DEF alive
  2. hp_delta           -- normalised HP difference
  3. util_delta         -- utility count difference
  4. eco_delta          -- weapon tier average difference
  5. spike_planted      -- binary
  6. time_pressure      -- seconds past 60s / 40
  7. map_prior          -- site-specific base rate offset from 0.46
  8. positioning_delta  -- attacker zone aggression - defender zone aggression
  9. defuser_pressure   -- defenders have defuser AND spike is planted

Known ground truth anchors (Valorant):
  Base attacker win rate:     46%
  5v4 player advantage:       +18%
  Spike planted (5v5):        +38%
  Full-buy vs eco:            +23%
  Executing (on-site):        +7%
  Defuser in position (plant): -8%
"""

import numpy as np
import json
from pathlib import Path

_CACHE = Path(__file__).parent.parent.parent / "data" / "calibrator_weights.json"
_N_FEATURES = 9

# -- Map/site attacker base win rates --
MAP_PRIORS = {
    "ascent":   {"A": 0.44, "B": 0.49, "default": 0.46},
    "bind":     {"A": 0.43, "B": 0.47, "default": 0.45},
    "haven":    {"A": 0.48, "B": 0.50, "C": 0.46, "default": 0.48},
    "split":    {"A": 0.42, "B": 0.51, "default": 0.46},
    "icebox":   {"A": 0.45, "B": 0.44, "default": 0.44},
    "fracture": {"A": 0.47, "B": 0.48, "default": 0.47},
    "pearl":    {"A": 0.46, "B": 0.47, "default": 0.46},
    "lotus":    {"A": 0.47, "B": 0.48, "C": 0.46, "default": 0.47},
    "sunset":   {"A": 0.45, "B": 0.48, "default": 0.46},
    "oregon":   {"default": 0.52},
    "clubhouse":{"default": 0.49},
    "consulate":{"default": 0.51},
    "villa":    {"default": 0.50},
    "default":  {"default": 0.46},
}

# -- Economy weapon tiers --
WEAPON_TIERS = {
    "classic": 0, "shorty": 0, "frenzy": 0, "ghost": 0, "sheriff": 1,
    "stinger": 1, "spectre": 1, "bucky": 1, "judge": 1,
    "bulldog": 2, "guardian": 2, "marshal": 2, "ares": 2, "odin": 2,
    "phantom": 3, "vandal": 3, "operator": 3, "outlaw": 3,
    "pistol": 0, "smg": 1, "shotgun": 1, "lmg": 1, "ar": 2, "dmr": 2, "sniper": 3,
}

def get_weapon_tier(weapon_name: str) -> int:
    return WEAPON_TIERS.get(weapon_name.lower(), 2)


def _generate_training_data(n: int = 20000, seed: int = 42) -> tuple:
    rng = np.random.default_rng(seed)
    X, y = [], []

    for _ in range(n):
        atk_alive  = rng.integers(1, 6)
        def_alive  = rng.integers(1, 6)
        atk_hp     = rng.integers(atk_alive * 20, atk_alive * 150 + 1)
        def_hp     = rng.integers(def_alive * 20, def_alive * 150 + 1)
        atk_util   = rng.integers(0, 10)
        def_util   = rng.integers(0, 10)
        atk_eco    = rng.integers(0, 4)
        def_eco    = rng.integers(0, 4)
        spike      = rng.random() < 0.35
        time_rem   = rng.integers(0, 100)
        base_prior = rng.uniform(0.42, 0.52)
        pos_delta  = rng.uniform(-0.6, 0.6)   # attacker zone aggression advantage
        has_def    = rng.random() < 0.65       # defenders usually carry defuser

        p = base_prior
        p += (atk_alive - def_alive) * 0.18
        p += (atk_hp - def_hp) / 500 * 0.12
        p += (atk_util - def_util) * 0.03
        p += (atk_eco - def_eco) * 0.06
        if spike:
            p += 0.38
        p -= max(0, time_rem - 60) * 0.003
        p += pos_delta * 0.07               # positioning effect on win probability
        if spike and has_def:
            p -= 0.08                       # defuser in position lowers post-plant rate
        p = np.clip(p, 0.02, 0.98)

        outcome = int(rng.random() < p)

        X.append([
            atk_alive - def_alive,
            (atk_hp - def_hp) / 500,
            atk_util - def_util,
            atk_eco - def_eco,
            int(spike),
            max(0, time_rem - 60) / 40,
            base_prior - 0.46,
            pos_delta,
            int(spike and has_def),
        ])
        y.append(outcome)

    return np.array(X), np.array(y)


def _fit() -> dict:
    from sklearn.linear_model import LogisticRegression
    from sklearn.preprocessing import StandardScaler

    X, y = _generate_training_data()
    scaler = StandardScaler()
    Xs = scaler.fit_transform(X)

    model = LogisticRegression(max_iter=500, C=1.0)
    model.fit(Xs, y)

    coef = model.coef_[0]
    return {
        "player_delta":   float(coef[0]),
        "hp_delta":       float(coef[1]),
        "util_delta":     float(coef[2]),
        "eco_delta":      float(coef[3]),
        "spike_planted":  float(coef[4]),
        "time_pressure":  float(coef[5]),
        "map_prior":      float(coef[6]),
        "positioning":    float(coef[7]),
        "defuser":        float(coef[8]),
        "intercept":      float(model.intercept_[0]),
        "scale_mean":     scaler.mean_.tolist(),
        "scale_std":      scaler.scale_.tolist(),
    }


def load_weights() -> dict:
    if _CACHE.exists():
        with open(_CACHE) as f:
            w = json.load(f)
        if len(w.get("scale_mean", [])) == _N_FEATURES:
            return w
    return fit_and_save()


def fit_and_save() -> dict:
    _CACHE.parent.mkdir(parents=True, exist_ok=True)
    w = _fit()
    with open(_CACHE, "w") as f:
        json.dump(w, f, indent=2)
    print(f"Calibrator fitted and saved -> {_CACHE}")
    return w


def predict_prob(features: list, weights: dict) -> float:
    mean = weights["scale_mean"]
    std  = weights["scale_std"]
    coef = [
        weights["player_delta"], weights["hp_delta"], weights["util_delta"],
        weights["eco_delta"],    weights["spike_planted"], weights["time_pressure"],
        weights["map_prior"],    weights["positioning"],   weights["defuser"],
    ]
    xs = [(features[i] - mean[i]) / (std[i] + 1e-8) for i in range(len(features))]
    logit = weights["intercept"] + sum(c * x for c, x in zip(coef, xs))
    return float(1 / (1 + np.exp(-logit)))
