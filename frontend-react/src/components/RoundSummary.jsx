import s from './RoundSummary.module.css'

function buildSummary(ticks) {
  if (!ticks || ticks.length < 2) return null

  const first = ticks[0]
  const last  = ticks[ticks.length - 1]
  const winner = last.blended_prob > 50 ? 'attack' : 'defense'

  // Biggest probability swing
  let maxSwing = 0, swingTick = null
  for (let i = 1; i < ticks.length; i++) {
    const delta = Math.abs(ticks[i].blended_prob - ticks[i - 1].blended_prob)
    if (delta > maxSwing) { maxSwing = delta; swingTick = ticks[i] }
  }

  // Lowest point for the winner
  const winnerProbs = ticks.map(t => winner === 'attack' ? t.blended_prob : 100 - t.blended_prob)
  const lowestPoint = Math.min(...winnerProbs)

  const bullets = []

  if (maxSwing > 8 && swingTick) {
    const dir = swingTick.blended_prob > 50 ? 'attackers' : 'defenders'
    bullets.push(`Biggest shift at t=${swingTick.time_remaining_s}s (${swingTick.phase}) — ${maxSwing.toFixed(1)}% swing toward ${dir}.`)
  }

  if (lowestPoint < 35) {
    bullets.push(`${winner === 'attack' ? 'Attackers' : 'Defenders'} fell to ${lowestPoint.toFixed(1)}% before recovering to win.`)
  }

  const topFactor = last.factors?.[0]
  if (topFactor) {
    bullets.push(`Decisive factor: ${topFactor.label} (${topFactor.delta >= 0 ? '+' : ''}${topFactor.delta?.toFixed(1)}%).`)
  }

  if (last.state_summary?.spike_planted) {
    bullets.push('Round ended post-plant — spike detonated or defused.')
  }

  if (bullets.length === 0) bullets.push('Clean round — no major probability swings recorded.')

  return { winner, bullets }
}

export default function RoundSummary({ ticks, teamAtk, teamDef }) {
  const summary = buildSummary(ticks)
  if (!summary) return null

  const { winner, bullets } = summary
  const winnerName = winner === 'attack' ? teamAtk : teamDef

  return (
    <div className={s.card}>
      <div className={s.header}>
        <span className={s.title}>Round Summary</span>
        <span className={`${s.winner} ${winner === 'attack' ? s.winnerAtk : s.winnerDef}`}>
          {winnerName} win
        </span>
      </div>
      <div className={s.bullets}>
        {bullets.map((b, i) => (
          <div key={i} className={s.bullet}>
            <span className={s.bulletIcon}>▲</span>
            {b}
          </div>
        ))}
      </div>
    </div>
  )
}
