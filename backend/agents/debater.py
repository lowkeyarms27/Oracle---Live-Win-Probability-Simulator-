"""
Adversarial Debater -- argues against the simulation with round history,
positioning, and defuser context.
"""
import json
from backend.state import RoundState
from backend.config import GEMINI_API_KEY, GEMINI_MODEL

try:
    from google import genai
    _client = genai.Client(api_key=GEMINI_API_KEY) if GEMINI_API_KEY else None
except Exception:
    _client = None


def _history_summary(history: list) -> str:
    if not history:
        return "No prior rounds recorded."
    lines = []
    for r in history[-3:]:
        lines.append(f"Round {r.get('round','-')}: {r.get('winner','?')} won "
                     f"(final atk prob {r.get('final_prob','?')}%)"
                     f"{' -- ' + r.get('key_event','') if r.get('key_event') else ''}")
    return " | ".join(lines)


_DEBATE_PROMPT = """You are an adversarial esports analyst. Argue AGAINST the simulation's conclusion.

SIMULATION SAYS:
- Attacker win probability: {win_prob_attack}%
- Phase: {phase} | Time remaining: {time_remaining}s | Map: {map} {site}-site
- Attackers: {atk_alive} alive, {atk_hp} HP, {atk_util} util, eco tier {atk_eco:.1f}/3
- Defenders: {dfn_alive} alive, {dfn_hp} HP, {dfn_util} util, eco tier {dfn_eco:.1f}/3
- Spike planted: {spike_planted} | Defenders have defuser: {has_defuser}
- Positioning delta (ATK forward = positive): {pos_delta:.2f}
- Top factors: {factors}
- Round history: {history}

Identify 2-3 hidden risks or overlooked factors (momentum, map geometry, clutch variance, defuser, positioning, tilt). Be specific about impact magnitude.

Respond ONLY in JSON:
{{"counter_narrative": "one punchy sentence", "adjusted_prob": number, "risks": [{{"risk": "...", "impact": "+/-N%", "reasoning": "..."}}]}}"""


def argue(state: RoundState, sim_result: dict) -> dict:
    fallback = _rule_based_debate(state, sim_result)
    if not _client:
        return fallback

    history_str = _history_summary(state.round_history)
    prompt = _DEBATE_PROMPT.format(
        win_prob_attack=sim_result["win_prob_attack"],
        phase=state.phase,
        time_remaining=state.time_remaining_s,
        map=state.map_name.title(),
        site=state.site,
        atk_alive=state.attackers_alive,
        atk_hp=state.attacker_hp_total,
        atk_util=state.attacker_util_count,
        atk_eco=state.attacker_eco_avg,
        dfn_alive=state.defenders_alive,
        dfn_hp=state.defender_hp_total,
        dfn_util=state.defender_util_count,
        dfn_eco=state.defender_eco_avg,
        spike_planted=state.spike_planted,
        has_defuser=state.defenders_have_defuser,
        pos_delta=state.positioning_delta,
        factors=json.dumps(sim_result.get("factors", [])[:3]),
        history=history_str,
    )

    try:
        resp = _client.models.generate_content(
            model=GEMINI_MODEL,
            contents=prompt,
            config={"response_mime_type": "application/json"},
        )
        data = json.loads(resp.text)
        data.setdefault("counter_narrative", fallback["counter_narrative"])
        data.setdefault("risks", fallback["risks"])
        return data
    except Exception:
        return fallback


def _rule_based_debate(state: RoundState, sim: dict) -> dict:
    prob     = sim["win_prob_attack"]
    risks    = []
    adjusted = prob

    # -- Momentum from round history --
    if state.round_history:
        recent_winners = [r.get("winner") for r in state.round_history[-3:]]
        atk_streak = sum(1 for w in recent_winners if w == "attack")
        def_streak = sum(1 for w in recent_winners if w == "defense")
        if def_streak == 3 and prob > 50:
            risks.append({
                "risk": "Defenders on a 3-round win streak (tilt/momentum)",
                "impact": "-6%",
                "reasoning": "3-round momentum shifts are statistically significant; teams on losing streaks convert lower than base rate.",
            })
            adjusted = max(prob - 6, 3)
        elif atk_streak == 3 and prob < 50:
            risks.append({
                "risk": "Attackers on a 3-round win streak (momentum ignored)",
                "impact": "+6%",
                "reasoning": "Model ignores psychological momentum. Hot teams convert at higher rates than base.",
            })
            adjusted = min(prob + 6, 97)

    # -- Defuser in position (post-plant) --
    if state.spike_planted and state.defenders_have_defuser:
        risks.append({
            "risk": "Defenders have defuser in position",
            "impact": "-8%",
            "reasoning": "With defuser in hand, defenders can attempt a defuse trade even from a losing position -- adds ~8% to their real-world post-plant conversion rate.",
        })
        adjusted = max(adjusted - 8, 3)

    # -- Positioning --
    if state.positioning_delta < -0.3 and not state.spike_planted:
        risks.append({
            "risk": "Defenders holding aggressive angles",
            "impact": "-5%",
            "reasoning": "Forward defender positioning cuts attacker rotation paths and reduces effective utility usage. Model underweights this geometry factor.",
        })
        adjusted = max(adjusted - 5, 3)
    elif state.positioning_delta > 0.5 and not state.spike_planted:
        risks.append({
            "risk": "Attackers committing to execute",
            "impact": "+5%",
            "reasoning": "Committed execute positioning historically converts at +5% above base when utility is available -- model is conservative on grouped-push scenarios.",
        })
        adjusted = min(adjusted + 5, 97)

    # -- Economy mismatch --
    if state.attacker_eco_avg < 1.0 and not state.spike_planted:
        risks.append({
            "risk": f"Attacker eco round (avg weapon tier {state.attacker_eco_avg:.1f})",
            "impact": "-18%",
            "reasoning": "Pistols/light buys vs rifles lose site takes at ~23% higher rate regardless of player count.",
        })
        adjusted = max(adjusted - 18, 3)

    # -- Map geometry --
    if state.map_name.lower() == "split" and state.site == "A" and not state.spike_planted:
        risks.append({
            "risk": "Split A-site: one of the hardest executes in Valorant",
            "impact": "-8%",
            "reasoning": "Split A has narrow choke points and elevated defender positions -- execute win rate is ~42% even with full utility.",
        })
        adjusted = max(adjusted - 8, 3)
    elif state.map_name.lower() == "bind" and state.spike_planted:
        risks.append({
            "risk": "Bind teleporter enables fast retakes post-plant",
            "impact": "-7%",
            "reasoning": "Both teleporters let defenders collapse within 3s. Post-plant success rate is lower than other maps.",
        })
        adjusted = max(adjusted - 7, 3)

    # -- Spike timer --
    if state.spike_planted and state.spike_time_remaining_s and state.spike_time_remaining_s < 5:
        risks.append({
            "risk": "Defuse window is critically tight (<5s)",
            "impact": "+8%",
            "reasoning": "With <5s on spike, a mid-defuse trade becomes impossible. Attackers post-plant win rate jumps.",
        })
        adjusted = min(adjusted + 8, 97)

    # -- 1vN clutch variance --
    if state.attackers_alive == 1 and state.defenders_alive >= 2:
        risks.append({
            "risk": "1vN clutch -- high variance outcome",
            "impact": "-10%",
            "reasoning": "Monte Carlo assumes sequential duels; real 1v2/3 crossfires are non-linear. Actual conversion is 8-15% lower.",
        })
        adjusted = max(adjusted - 10, 3)

    # -- Time + no plant --
    if state.time_remaining_s < 20 and not state.spike_planted and state.attackers_alive > 0:
        risks.append({
            "risk": "Forced execute with <20s remaining",
            "impact": "-14%",
            "reasoning": "Rushed executes with no time for post-plant give defenders a free defuse window. Tournament data shows <15% success rate.",
        })
        adjusted = max(adjusted - 14, 3)

    if not risks:
        risks.append({
            "risk": "Psychological variance / read advantage",
            "impact": "+/-5%",
            "reasoning": "Model is agnostic to in-game reads, bait-and-switch tactics, and individual confidence levels.",
        })

    narrative = _build_narrative(state, prob, adjusted, risks)
    return {
        "counter_narrative": narrative,
        "adjusted_prob": round(adjusted, 1),
        "risks": risks[:3],
    }


def _build_narrative(state, prob, adjusted, risks) -> str:
    delta = abs(prob - adjusted)
    if delta < 3:
        return f"Model says {prob:.0f}% -- risks are balanced, no dominant counter-factor."
    primary = risks[0]["risk"].split("(")[0].strip().lower() if risks else "variance"
    direction = "lower" if adjusted < prob else "higher"
    return f"Model says {prob:.0f}% but {primary} pushes real odds {direction} -- actual closer to {adjusted:.0f}%."
