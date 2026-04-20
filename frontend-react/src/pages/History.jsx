import { MOCK_TICKS, MOCK_MATCH } from '../data/mockMatch'
import TimelineChart from '../components/TimelineChart'
import s from './History.module.css'

const PAST_ROUNDS = [
  { round: 10, winner: 'attack', final_prob: 78.4, duration: 85, team_attack: 'Sentinels', team_defense: 'LOUD', map: 'Ascent', key_moment: 'Post-plant 3v2 secured' },
  { round: 11, winner: 'defense', final_prob: 31.2, duration: 95, team_attack: 'Sentinels', team_defense: 'LOUD', map: 'Ascent', key_moment: 'Eco round -- LOUD full-buy dominated' },
  { round: 12, winner: 'attack', final_prob: 62.9, duration: 72, team_attack: 'Sentinels', team_defense: 'LOUD', map: 'Ascent', key_moment: 'Fast execute caught LOUD off-rotation' },
  { round: 13, winner: 'attack', final_prob: 89.5, duration: 45, team_attack: 'Sentinels', team_defense: 'LOUD', map: 'Ascent', key_moment: '2v1 post-plant -- spike detonated' },
]

export default function History() {
  return (
    <div className={s.page}>
      <div className={s.header}>
        <div className={s.eyebrow}>Round History</div>
        <div className={s.title}>Match Log</div>
        <div className={s.sub}>{MOCK_MATCH.team_attack} vs {MOCK_MATCH.team_defense} · {MOCK_MATCH.map}</div>
      </div>

      <div className={s.timelineCard}>
        <div className={s.cardTitle}>Round 13 — Full Probability Timeline</div>
        <TimelineChart ticks={MOCK_TICKS} />
      </div>

      <div className={s.roundList}>
        {PAST_ROUNDS.map(r => (
          <div key={r.round} className={`${s.roundCard} ${r.winner === 'attack' ? s.roundWinAtk : s.roundWinDef}`}>
            <div className={s.roundHeader}>
              <span className={s.roundNum}>Round {r.round}</span>
              <span className={`${s.roundWinner} ${r.winner === 'attack' ? s.winnerAtk : s.winnerDef}`}>
                {r.winner === 'attack' ? r.team_attack : r.team_defense} win
              </span>
              <span className={s.roundDuration}>{r.duration}s</span>
            </div>
            <div className={s.roundBody}>
              <div className={s.roundMoment}>{r.key_moment}</div>
              <div className={`${s.roundProb} ${r.winner === 'attack' ? s.probAtk : s.probDef}`}>
                {r.winner === 'attack' ? r.final_prob : (100 - r.final_prob).toFixed(1)}%
              </div>
            </div>
            <div className={s.probBar}>
              <div
                className={s.probFill}
                style={{ width: `${r.final_prob}%`, background: r.winner === 'attack' ? 'var(--accent)' : 'var(--red)' }}
              />
            </div>
          </div>
        ))}
      </div>
    </div>
  )
}
