import s from './StatePanel.module.css'

const POSITIONING_LABELS = {
  execute:     'Executing',
  approach:    'Approaching',
  even:        'Even',
  def_forward: 'DEF Forward',
}

export default function StatePanel({ state }) {
  if (!state) return null
  const { atk_alive, def_alive, atk_hp, def_hp, atk_util, def_util,
          spike_planted, has_defuser, positioning } = state
  const maxHp = 500

  return (
    <div className={s.panel}>
      <div className={`${s.side} ${s.sideAtk}`}>
        <div className={`${s.sideTitle} ${s.atkTitle}`}>Attack</div>
        <div className={s.stat}><span className={s.statLabel}>Players alive</span><span className={s.statValue}>{atk_alive}/5</span></div>
        <div className={s.stat}><span className={s.statLabel}>Total HP</span><span className={s.statValue}>{atk_hp}</span></div>
        <div className={s.hpBar}><div className={s.hpFillAtk} style={{ width: `${Math.min(atk_hp / maxHp * 100, 100)}%` }} /></div>
        <div className={s.stat} style={{ marginTop: 8 }}><span className={s.statLabel}>Utility pieces</span><span className={s.statValue}>{atk_util}</span></div>
      </div>
      <div className={`${s.side} ${s.sideDef}`}>
        <div className={`${s.sideTitle} ${s.defTitle}`}>Defense</div>
        <div className={s.stat}><span className={s.statLabel}>Players alive</span><span className={s.statValue}>{def_alive}/5</span></div>
        <div className={s.stat}><span className={s.statLabel}>Total HP</span><span className={s.statValue}>{def_hp}</span></div>
        <div className={s.hpBar}><div className={s.hpFillDef} style={{ width: `${Math.min(def_hp / maxHp * 100, 100)}%` }} /></div>
        <div className={s.stat} style={{ marginTop: 8 }}><span className={s.statLabel}>Utility pieces</span><span className={s.statValue}>{def_util}</span></div>
        {has_defuser && (
          <div className={s.defuserBadge}>Defuser in hand</div>
        )}
      </div>
      {spike_planted && (
        <div className={s.spike}>
          <span className={s.spikeDot} /> Spike Planted
        </div>
      )}
      {positioning && positioning !== 'even' && (
        <div className={s.positioning}>
          <span className={s.posLabel}>Positioning</span>
          <span className={s.posValue}>{POSITIONING_LABELS[positioning] || positioning}</span>
        </div>
      )}
    </div>
  )
}
