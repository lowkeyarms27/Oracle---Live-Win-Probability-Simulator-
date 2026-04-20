import s from './DebaterCard.module.css'

export default function DebaterCard({ debate, simProb }) {
  if (!debate) return null
  const { counter_narrative, adjusted_prob, risks = [] } = debate
  const delta = adjusted_prob - simProb
  const sign  = delta >= 0 ? '+' : ''

  return (
    <div className={s.card}>
      <div className={s.header}>
        <span className={s.icon}>⚡</span>
        <span className={s.title}>Adversarial Debater</span>
        <span className={s.adjProb}>{adjusted_prob?.toFixed(1)}% ({sign}{delta?.toFixed(1)}%)</span>
      </div>
      {counter_narrative && <p className={s.narrative}>"{counter_narrative}"</p>}
      {risks.length > 0 && (
        <div className={s.risks}>
          {risks.map((r, i) => (
            <div key={i} className={s.risk}>
              <div className={s.riskHeader}>
                <span className={s.riskName}>{r.risk}</span>
                <span className={s.riskImpact}>{r.impact}</span>
              </div>
              <div className={s.riskReason}>{r.reasoning}</div>
            </div>
          ))}
        </div>
      )}
    </div>
  )
}
