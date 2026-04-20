from dataclasses import dataclass, field, asdict
from typing import List, Optional
import time

ZONE_AGGRESSION = {
    "a-site": 1.00, "b-site": 1.00, "c-site": 1.00,
    "a-main": 0.70, "b-main": 0.70,
    "catwalk": 0.60, "b-short": 0.65,
    "mid": 0.50,
    "ct": 0.20, "spawn": 0.10,
    "default": 0.50,
}


@dataclass
class PlayerState:
    id: str
    team: str
    alive: bool
    hp: int
    shield: int = 0
    has_spike: bool = False
    has_defuser: bool = False
    utilities: List[str] = field(default_factory=list)
    ultimate_ready: bool = False
    position: Optional[str] = None   # zone name e.g. "a-site", "mid", "ct"
    weapon: str = "rifle"
    weapon_tier: int = 2             # 0=pistol 1=light 2=heavy 3=rifle/op


@dataclass
class RoundState:
    game: str
    match_id: str
    round_number: int
    phase: str
    time_remaining_s: int
    spike_planted: bool
    spike_time_remaining_s: Optional[int]

    team_attack: str
    team_defense: str

    map_name: str = "default"
    site: str = "default"

    players: List[PlayerState] = field(default_factory=list)

    attackers_alive: int = 0
    defenders_alive: int = 0
    attacker_hp_total: int = 0
    defender_hp_total: int = 0
    attacker_util_count: int = 0
    defender_util_count: int = 0
    attacker_eco_avg: float = 2.0
    defender_eco_avg: float = 2.0
    positioning_delta: float = 0.0      # atk zone aggression - def zone aggression
    defenders_have_defuser: bool = False

    score_attack: int = 0
    score_defense: int = 0

    round_history: List[dict] = field(default_factory=list)

    timestamp: float = field(default_factory=time.time)

    def compute_derived(self):
        atk = [p for p in self.players if p.team == "attack"]
        dfn = [p for p in self.players if p.team == "defense"]

        self.attackers_alive     = sum(1 for p in atk if p.alive)
        self.defenders_alive     = sum(1 for p in dfn if p.alive)
        self.attacker_hp_total   = sum(p.hp for p in atk if p.alive)
        self.defender_hp_total   = sum(p.hp for p in dfn if p.alive)
        self.attacker_util_count = sum(len(p.utilities) for p in atk if p.alive)
        self.defender_util_count = sum(len(p.utilities) for p in dfn if p.alive)

        atk_tiers = [p.weapon_tier for p in atk if p.alive]
        dfn_tiers = [p.weapon_tier for p in dfn if p.alive]
        self.attacker_eco_avg = sum(atk_tiers) / max(len(atk_tiers), 1)
        self.defender_eco_avg = sum(dfn_tiers) / max(len(dfn_tiers), 1)

        # Positioning: zone aggression score (higher = more attacker-forward)
        atk_zones = [ZONE_AGGRESSION.get(p.position or "default", 0.5)
                     for p in atk if p.alive]
        def_zones = [ZONE_AGGRESSION.get(p.position or "default", 0.5)
                     for p in dfn if p.alive]
        if any(p.position for p in self.players if p.alive):
            atk_agg = sum(atk_zones) / max(len(atk_zones), 1)
            def_agg = sum(def_zones) / max(len(def_zones), 1)
            self.positioning_delta = round(atk_agg - def_agg, 3)
        # else: leave at whatever was set externally (e.g. from scenario params)

        self.defenders_have_defuser = any(p.has_defuser for p in dfn if p.alive)

    def to_dict(self):
        self.compute_derived()
        return asdict(self)
