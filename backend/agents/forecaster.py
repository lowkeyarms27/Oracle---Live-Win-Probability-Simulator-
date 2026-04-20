"""
Forecaster -- orchestrates simulator + debater, outputs final Oracle prediction.
Also maintains a rolling probability timeline.
"""
import time
from collections import deque
from backend.state import RoundState
from backend.agents import simulator, debater


_HISTORY_MAX = 120   # 120 x 5s = 10 minutes of history per match


class MatchForecaster:
    def __init__(self, match_id: str, game: str):
        self.match_id   = match_id
        self.game       = game
        self.timeline   = deque(maxlen=_HISTORY_MAX)
        self.last_state = None

    def update(self, state: RoundState) -> dict:
        self.last_state = state

        sim    = simulator.run(state)
        debate = debater.argue(state, sim)

        # Blend: 70% simulation, 30% adversarial adjusted
        blended = round(
            0.70 * sim["win_prob_attack"] + 0.30 * debate["adjusted_prob"], 1
        )

        tick = {
            "ts":                  time.time(),
            "round":               state.round_number,
            "phase":               state.phase,
            "time_remaining_s":    state.time_remaining_s,
            "win_prob_attack":     sim["win_prob_attack"],
            "win_prob_defense":    sim["win_prob_defense"],
            "blended_prob":        blended,
            "confidence":          sim["confidence"],
            "base_prob":           sim["base_prob"],
            "factors":             sim["factors"],
            "debate": {
                "counter_narrative": debate["counter_narrative"],
                "adjusted_prob":     debate["adjusted_prob"],
                "risks":             debate.get("risks", []),
            },
            "state_summary": {
                "atk_alive":        state.attackers_alive,
                "def_alive":        state.defenders_alive,
                "atk_hp":           state.attacker_hp_total,
                "def_hp":           state.defender_hp_total,
                "atk_util":         state.attacker_util_count,
                "def_util":         state.defender_util_count,
                "spike_planted":    state.spike_planted,
            },
        }

        self.timeline.append(tick)
        return tick

    def get_timeline(self) -> list:
        return list(self.timeline)


# Global registry -- one forecaster per active match
_registry: dict[str, MatchForecaster] = {}


def get_forecaster(match_id: str, game: str = "valorant") -> MatchForecaster:
    if match_id not in _registry:
        _registry[match_id] = MatchForecaster(match_id, game)
    return _registry[match_id]


def clear_match(match_id: str):
    _registry.pop(match_id, None)
