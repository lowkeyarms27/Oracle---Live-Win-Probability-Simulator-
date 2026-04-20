import {
  LineChart, Line, XAxis, YAxis, Tooltip,
  ReferenceLine, ResponsiveContainer, Area, AreaChart,
} from 'recharts'
import s from './TimelineChart.module.css'

function CustomTooltip({ active, payload, label }) {
  if (!active || !payload?.length) return null
  const d = payload[0]?.payload
  return (
    <div style={{
      background: '#141414', border: '1px solid #2a2a2a', borderRadius: 8,
      padding: '8px 12px', fontSize: 12,
    }}>
      <div style={{ color: '#555', marginBottom: 4 }}>t={label}s · {d?.phase}</div>
      <div style={{ color: '#00ff88', fontFamily: 'var(--font-mono)', fontWeight: 700 }}>
        ATK {payload[0]?.value?.toFixed(1)}%
      </div>
      {d?.debate?.counter_narrative && (
        <div style={{ color: '#555', marginTop: 4, maxWidth: 200, lineHeight: 1.4 }}>
          {d.debate.counter_narrative}
        </div>
      )}
    </div>
  )
}

export default function TimelineChart({ ticks }) {
  if (!ticks?.length) return null

  const data = ticks.map((t, i) => ({
    ...t,
    t: i * 5,
    atk: t.blended_prob ?? t.win_prob_attack,
  }))

  return (
    <div className={s.wrap}>
      <div className={s.title}>Win Probability Timeline</div>
      <ResponsiveContainer width="100%" height={180}>
        <AreaChart data={data} margin={{ top: 4, right: 4, bottom: 0, left: -20 }}>
          <defs>
            <linearGradient id="atkGrad" x1="0" y1="0" x2="0" y2="1">
              <stop offset="5%"  stopColor="#00ff88" stopOpacity={0.2} />
              <stop offset="95%" stopColor="#00ff88" stopOpacity={0} />
            </linearGradient>
          </defs>
          <XAxis dataKey="t" tick={{ fill: '#555', fontSize: 9 }} axisLine={false} tickLine={false} unit="s" />
          <YAxis domain={[0, 100]} tick={{ fill: '#555', fontSize: 9 }} axisLine={false} tickLine={false} unit="%" />
          <Tooltip content={<CustomTooltip />} />
          <ReferenceLine y={50} stroke="#2a2a2a" strokeDasharray="4 3" />
          <Area
            type="monotone" dataKey="atk"
            stroke="#00ff88" strokeWidth={2}
            fill="url(#atkGrad)" dot={false} activeDot={{ r: 4, fill: '#00ff88' }}
          />
        </AreaChart>
      </ResponsiveContainer>
      <div className={s.legend}>
        <div className={s.legendItem}><span className={s.dot} style={{ background: '#00ff88' }} /> Attacker win %</div>
        <div className={s.legendItem}><span className={s.dot} style={{ background: '#2a2a2a', border: '1px dashed #555' }} /> 50% baseline</div>
      </div>
    </div>
  )
}
