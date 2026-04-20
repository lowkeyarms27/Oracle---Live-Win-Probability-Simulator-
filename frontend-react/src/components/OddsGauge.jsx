import s from './OddsGauge.module.css'

export default function OddsGauge({ atkProb, defProb, confidence, teamAtk, teamDef }) {
  const atk = atkProb ?? 50
  const def = defProb ?? 50
  const conf = (confidence ?? 0.7) * 100

  const probClass = atk > 55 ? s.probAtk : atk < 45 ? s.probDef : s.probEven
  const leadTeam  = atk > 50 ? teamAtk : atk < 50 ? teamDef : null
  const leadProb  = atk > 50 ? atk : def

  return (
    <div className={s.wrap}>
      <div className={s.teamLabels}>
        <span className={s.teamLabel + ' ' + s.teamAtk}>ATK · {teamAtk}</span>
        <span className={s.teamLabel + ' ' + s.teamDef}>DEF · {teamDef}</span>
      </div>

      <div style={{ textAlign: 'center' }}>
        <div className={`${s.mainProb} ${probClass}`}>
          {leadTeam ? `${leadProb.toFixed(1)}%` : '50%'}
        </div>
        <div className={s.probLabel}>
          {leadTeam ? `${leadTeam} win probability` : 'Even match'}
        </div>
      </div>

      <div style={{ width: '100%' }}>
        <div className={s.bar}>
          <div className={s.barFill} style={{ width: `${atk}%` }} />
        </div>
        <div className={s.barLabels}>
          <span className={s.atkPct}>{atk.toFixed(1)}%</span>
          <span className={s.defPct}>{def.toFixed(1)}%</span>
        </div>
      </div>

      <div style={{ width: '100%' }}>
        <div className={s.confidence}>Model confidence: {conf.toFixed(0)}%</div>
        <div className={s.confBar}>
          <div className={s.confFill} style={{ width: `${conf}%` }} />
        </div>
      </div>
    </div>
  )
}
