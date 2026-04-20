"""
Monte Carlo simulator -- 9-sensor logistic regression calibrated model.
"""
import random
from backend.config import MC_ITERATIONS
from backend.state import RoundState
from backend.ml.calibrator import load_weights, MAP_PRIORS, predict_prob

_weights = None

def _get_weights():
    global _weights
    if _weights is None:
        _weights = load_weights()
    return _weights


def _map_prior(state: RoundState) -> float:
    priors = MAP_PRIORS.get(state.map_name.lower(), MAP_PRIORS["default"])
    return priors.get(state.site.upper(), priors.get("default", 0.46))


def _build_features(state: RoundState, map_prior: float) -> list:
    return [
        state.attackers_alive - state.defenders_alive,                          # 1
        (state.attacker_hp_total - state.defender_hp_total) / 500,              # 2
        state.attacker_util_count - state.defender_util_count,                  # 3
        state.attacker_eco_avg - state.defender_eco_avg,                        # 4
        int(state.spike_planted),                                               # 5
        max(0, state.time_remaining_s - 60) / 40,                              # 6
        map_prior - 0.46,                                                       # 7
        state.positioning_delta,                                                # 8
        int(state.spike_planted and state.defenders_have_defuser),              # 9
    ]


def _base_probability(state: RoundState) -> float:
    w         = _get_weights()
    map_prior = _map_prior(state)
    features  = _build_features(state, map_prior)
    return max(0.02, min(0.98, predict_prob(features, w)))


def _simulate_one(base_prob: float, state: RoundState) -> bool:
    prob      = base_prob
    atk_alive = state.attackers_alive
    dfn_alive = state.defenders_alive

    while atk_alive > 0 and dfn_alive > 0:
        duel_prob = min(0.78, max(0.22, prob))
        if random.random() < duel_prob:
            dfn_alive -= 1
        else:
            atk_alive -= 1

    if state.spike_planted and atk_alive == 0 and dfn_alive > 0:
        defuse_time = state.spike_time_remaining_s or 7
        return defuse_time < 3

    return atk_alive > 0


def run(state: RoundState, iterations: int = MC_ITERATIONS) -> dict:
    state.compute_derived()
    base      = _base_probability(state)
    map_prior = _map_prior(state)

    wins     = sum(_simulate_one(base, state) for _ in range(iterations))
    win_prob = wins / iterations

    data_richness = min(1.0, (state.attackers_alive + state.defenders_alive) / 10)
    round_conf    = min(1.0, len(state.round_history) / 5)
    confidence    = round(0.55 + 0.30 * data_richness + 0.10 * round_conf, 2)

    factors = _explain(state, base, win_prob, map_prior)

    return {
        "win_prob_attack":  round(win_prob * 100, 1),
        "win_prob_defense": round((1 - win_prob) * 100, 1),
        "base_prob":        round(base * 100, 1),
        "map_prior":        round(map_prior * 100, 1),
        "confidence":       confidence,
        "iterations":       iterations,
        "phase":            state.phase,
        "factors":          factors,
    }


def _explain(state: RoundState, base: float, final: float, map_prior: float) -> list:
    factors = []

    map_delta = (map_prior - 0.46) * 100
    if abs(map_delta) > 0.5:
        factors.append({
            "label": f"Map prior ({state.map_name.title()} {state.site}-site)",
            "delta": round(map_delta, 1),
        })

    pd = state.attackers_alive - state.defenders_alive
    if pd != 0:
        factors.append({
            "label": f"{'Attacker' if pd > 0 else 'Defender'} player advantage ({state.attackers_alive}v{state.defenders_alive})",
            "delta": round(pd * 18, 1),
        })

    hd = state.attacker_hp_total - state.defender_hp_total
    if abs(hd) > 25:
        factors.append({
            "label": f"HP lead ({'ATK' if hd > 0 else 'DEF'} +{abs(hd)})",
            "delta": round(hd / 500 * 12 * 100, 1),
        })

    ed = state.attacker_eco_avg - state.defender_eco_avg
    if abs(ed) > 0.4:
        factors.append({
            "label": "Full-buy advantage" if ed > 0 else "Eco deficit",
            "delta": round(ed * 6 * 100, 1),
        })

    if state.spike_planted:
        factors.append({"label": "Spike planted", "delta": 38.0})

    ud = state.attacker_util_count - state.defender_util_count
    if abs(ud) > 0:
        factors.append({
            "label": f"Utility edge ({'ATK' if ud > 0 else 'DEF'} +{abs(ud)} pieces)",
            "delta": round(ud * 3, 1),
        })

    pos = state.positioning_delta
    if abs(pos) > 0.15:
        factors.append({
            "label": "ATK positional advantage" if pos > 0 else "DEF positional advantage",
            "delta": round(pos * 7 * 100, 1),
        })

    if state.spike_planted and state.defenders_have_defuser:
        factors.append({"label": "Defuser in position (DEF)", "delta": -8.0})

    t = state.time_remaining_s
    if t > 60:
        factors.append({
            "label": f"Time pressure ({t}s remaining)",
            "delta": round(-max(0, t - 60) * 0.3, 1),
        })

    factors.sort(key=lambda x: abs(x["delta"]), reverse=True)
    return factors[:5]
