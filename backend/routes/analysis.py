import asyncio
import json
import uuid
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pydantic import BaseModel
from typing import List, Optional

from backend.state import RoundState, PlayerState
from backend.agents.forecaster import get_forecaster, clear_match
from backend.ml.calibrator import get_weapon_tier

router = APIRouter(prefix="/api")

# Maps positioning label -> (atk zone, def zone)
POSITIONING_ZONES = {
    "execute":     ("a-site",   "ct"),
    "approach":    ("a-main",   "ct"),
    "even":        ("default",  "default"),
    "def_forward": ("spawn",    "mid"),
}


class PlayerIn(BaseModel):
    id: str
    team: str
    alive: bool
    hp: int
    shield: int = 0
    has_spike: bool = False
    has_defuser: bool = False
    utilities: List[str] = []
    ultimate_ready: bool = False
    position: Optional[str] = None
    weapon: str = "rifle"
    weapon_tier: Optional[int] = None


class StateIn(BaseModel):
    game: str = "valorant"
    match_id: str
    round_number: int = 1
    phase: str = "mid"
    time_remaining_s: int = 60
    spike_planted: bool = False
    spike_time_remaining_s: Optional[int] = None
    team_attack: str = "Team A"
    team_defense: str = "Team B"
    map_name: str = "ascent"
    site: str = "default"
    score_attack: int = 0
    score_defense: int = 0
    round_history: List[dict] = []
    players: List[PlayerIn] = []


def _build_state(body: StateIn) -> RoundState:
    players = []
    for p in body.players:
        tier = p.weapon_tier if p.weapon_tier is not None else get_weapon_tier(p.weapon)
        players.append(PlayerState(
            id=p.id, team=p.team, alive=p.alive, hp=p.hp,
            shield=p.shield, has_spike=p.has_spike, has_defuser=p.has_defuser,
            utilities=p.utilities, ultimate_ready=p.ultimate_ready,
            position=p.position, weapon=p.weapon, weapon_tier=tier,
        ))
    state = RoundState(
        game=body.game, match_id=body.match_id, round_number=body.round_number,
        phase=body.phase, time_remaining_s=body.time_remaining_s,
        spike_planted=body.spike_planted, spike_time_remaining_s=body.spike_time_remaining_s,
        team_attack=body.team_attack, team_defense=body.team_defense,
        map_name=body.map_name, site=body.site,
        score_attack=body.score_attack, score_defense=body.score_defense,
        round_history=body.round_history, players=players,
    )
    state.compute_derived()
    return state


@router.post("/predict")
async def predict(body: StateIn):
    state = _build_state(body)
    fc    = get_forecaster(body.match_id, body.game)
    return fc.update(state)


@router.get("/timeline/{match_id}")
async def timeline(match_id: str):
    fc = get_forecaster(match_id)
    return {"match_id": match_id, "timeline": fc.get_timeline()}


@router.delete("/match/{match_id}")
async def end_match(match_id: str):
    clear_match(match_id)
    return {"cleared": match_id}


@router.get("/simulate/scenario")
async def simulate_scenario(
    game: str = "valorant",
    map_name: str = "ascent",
    site: str = "default",
    atk_alive: int = 3,
    def_alive: int = 3,
    atk_hp: int = 270,
    def_hp: int = 270,
    atk_util: int = 3,
    def_util: int = 3,
    atk_eco: float = 2.0,
    def_eco: float = 2.0,
    spike_planted: bool = False,
    time_remaining_s: int = 60,
    phase: str = "mid",
    positioning: str = "even",
    has_defuser: bool = False,
    round_history: str = "[]",
):
    match_id = f"sim-{uuid.uuid4().hex[:8]}"
    history  = json.loads(round_history)
    atk_zone, def_zone = POSITIONING_ZONES.get(positioning, ("default", "default"))

    players = []
    for i in range(atk_alive):
        players.append(PlayerState(
            id=f"atk-{i}", team="attack", alive=True,
            hp=atk_hp // max(atk_alive, 1), shield=50,
            has_spike=(i == 0),
            utilities=["smoke"] * (atk_util // max(atk_alive, 1)),
            weapon_tier=round(atk_eco), position=atk_zone,
        ))
    for i in range(def_alive):
        players.append(PlayerState(
            id=f"def-{i}", team="defense", alive=True,
            hp=def_hp // max(def_alive, 1), shield=50,
            has_defuser=(i == 0 and has_defuser),
            utilities=["smoke"] * (def_util // max(def_alive, 1)),
            weapon_tier=round(def_eco), position=def_zone,
        ))

    state = RoundState(
        game=game, match_id=match_id, round_number=1,
        phase=phase, time_remaining_s=time_remaining_s,
        spike_planted=spike_planted, spike_time_remaining_s=35 if spike_planted else None,
        team_attack="Team A", team_defense="Team B",
        map_name=map_name, site=site,
        round_history=history, players=players,
    )
    state.compute_derived()

    from backend.agents.forecaster import MatchForecaster
    return MatchForecaster(match_id, game).update(state)


@router.get("/maps")
async def list_maps():
    from backend.ml.calibrator import MAP_PRIORS
    return {"maps": list(MAP_PRIORS.keys())}


@router.websocket("/ws/{match_id}")
async def ws_match(websocket: WebSocket, match_id: str):
    await websocket.accept()
    fc = get_forecaster(match_id)
    try:
        while True:
            try:
                raw   = await asyncio.wait_for(websocket.receive_text(), timeout=0.1)
                body  = StateIn(**json.loads(raw))
                body.match_id = match_id
                state = _build_state(body)
                tick  = fc.update(state)
                await websocket.send_json(tick)
            except asyncio.TimeoutError:
                if fc.timeline:
                    await websocket.send_json({"type": "heartbeat"})
                await asyncio.sleep(0.5)
    except WebSocketDisconnect:
        pass
