import { useState } from 'react'
import { SCENARIOS } from '../data/mockMatch'
import OddsGauge from '../components/OddsGauge'
import DebaterCard from '../components/DebaterCard'
import StatePanel from '../components/StatePanel'
import { simulateScenario } from '../lib/api'
import s from './Simulate.module.css'

const MAP_PRIORS = {
  ascent:{ A:0.44, B:0.49 }, bind:{ A:0.43, B:0.47 }, haven:{ A:0.48, B:0.50, C:0.46 },
  split:{ A:0.42, B:0.51 }, icebox:{ A:0.45, B:0.44 }, fracture:{ A:0.47, B:0.48 },
  pearl:{ A:0.46, B:0.47 }, lotus:{ A:0.47, B:0.48, C:0.46 }, sunset:{ A:0.45, B:0.48 },
}

const POSITIONING_OPTS = [
  { value: 'execute',     label: 'ATK Executing (on-site)',   delta:  0.70 },
  { value: 'approach',    label: 'ATK Approaching (main)',    delta:  0.30 },
  { value: 'even',        label: 'Even / Mid-fight',          delta:  0.00 },
  { value: 'def_forward', label: 'DEF Playing Forward',       delta: -0.40 },
]

function clientSim(params) {
  const { atk_alive, def_alive, atk_hp, def_hp, atk_util, def_util,
          atk_eco = 2, def_eco = 2, spike_planted, time_remaining_s,
          map_name = 'ascent', site = 'B',
          positioning = 'even', has_defuser = false,
          round_history = [] } = params

  const mapPrior = (MAP_PRIORS[map_name] || {})[site] || 0.46
  const posDelta = POSITIONING_OPTS.find(o => o.value === positioning)?.delta ?? 0
  const recentWinners = round_history.map(r => r.winner)
  const atkStreak = recentWinners.filter(w => w === 'attack').length
  const defStreak = recentWinners.filter(w => w === 'defense').length

  let prob = mapPrior
  prob += (atk_alive - def_alive) * 0.18
  prob += (atk_hp - def_hp) * 0.0012
  prob += (atk_util - def_util) * 0.03
  prob += (atk_eco - def_eco) * 0.06
  if (spike_planted) prob += 0.38
  prob -= Math.max(0, time_remaining_s - 60) * 0.003
  prob += posDelta * 0.07
  if (spike_planted && has_defuser) prob -= 0.08
  if (defStreak === recentWinners.length && defStreak >= 2) prob -= 0.06
  if (atkStreak === recentWinners.length && atkStreak >= 2) prob += 0.06
  prob = Math.max(0.03, Math.min(0.97, prob))

  const wins    = Array.from({ length: 1000 }, () => Math.random() < prob).filter(Boolean).length
  const winProb = wins / 10

  const factors = []
  const pd = atk_alive - def_alive
  if (pd !== 0) factors.push({ label: `${pd > 0 ? 'Attacker' : 'Defender'} player advantage`, delta: pd * 15 })
  const hd = atk_hp - def_hp
  if (Math.abs(hd) > 20) factors.push({ label: `HP lead (${hd > 0 ? 'ATK' : 'DEF'} +${Math.abs(hd)} HP)`, delta: hd * 0.12 })
  if (spike_planted) factors.push({ label: 'Spike planted', delta: 35 })
  if (Math.abs(posDelta) > 0.1) factors.push({ label: posDelta > 0 ? 'ATK positional advantage' : 'DEF positional advantage', delta: posDelta * 700 })
  if (spike_planted && has_defuser) factors.push({ label: 'Defuser in position (DEF)', delta: -8 })

  const risks = []
  if (atk_util === 0 && !spike_planted) risks.push({ risk: 'Zero attacker utility for execute', impact: '-10%', reasoning: 'Execute into a held site with no smokes/flashes is a coin-flip.' })
  if (atk_alive < def_alive) risks.push({ risk: 'Player count deficit', impact: `-${(def_alive - atk_alive) * 12}%`, reasoning: 'Multi-way crossfire disadvantage compounds with each second.' })
  if (time_remaining_s < 20 && !spike_planted) risks.push({ risk: 'Time running out', impact: '-15%', reasoning: 'Forced execute with <20s dramatically lowers conversion rate.' })
  if (spike_planted && has_defuser) risks.push({ risk: 'Defenders have defuser in position', impact: '-8%', reasoning: 'With defuser in hand, defenders can attempt a defuse trade even from a losing position.' })
  if (defStreak === recentWinners.length && defStreak >= 2) risks.push({ risk: `DEF ${defStreak}-round win streak (momentum)`, impact: '-6%', reasoning: 'Teams on winning streaks convert at higher rates -- model ignores psychological momentum.' })
  if (atkStreak === recentWinners.length && atkStreak >= 2) risks.push({ risk: `ATK ${atkStreak}-round win streak (momentum)`, impact: '+6%', reasoning: 'Attacking momentum from prior rounds boosts conversion above base rate.' })
  if (!risks.length) risks.push({ risk: 'Momentum / tilt factor', impact: '+/-5%', reasoning: "Model doesn't account for psychological pressure." })

  const adjProb = Math.max(3, Math.min(97, winProb + risks.reduce((acc, r) => acc + parseFloat(r.impact), 0) * 0.3))

  return {
    win_prob_attack: winProb,
    win_prob_defense: 100 - winProb,
    blended_prob: winProb * 0.7 + adjProb * 0.3,
    confidence: 0.55 + 0.4 * Math.min((atk_alive + def_alive) / 10, 1),
    factors: factors.slice(0, 5),
    debate: {
      counter_narrative: `Model says ${winProb.toFixed(1)}% -- ${risks[0]?.reasoning || 'variance is still high.'}`,
      adjusted_prob: adjProb,
      risks,
    },
  }
}

export default function Simulate() {
  const [activePreset, setActivePreset] = useState(null)
  const [form, setForm] = useState({
    game: 'valorant', map_name: 'ascent', site: 'B',
    atk_alive: 3, def_alive: 3,
    atk_hp: 270, def_hp: 270, atk_util: 3, def_util: 3,
    atk_eco: 2.0, def_eco: 2.0,
    spike_planted: false, time_remaining_s: 60, phase: 'mid',
    positioning: 'even', has_defuser: false,
    round_history: [],
  })
  const [result,  setResult]  = useState(null)
  const [loading, setLoading] = useState(false)
  const [offline, setOffline] = useState(false)

  function applyPreset(p) {
    setActivePreset(p.id)
    setForm({ ...form, ...p })
    setResult(null)
  }

  function set(key, val) {
    setActivePreset(null)
    setForm(f => ({ ...f, [key]: val }))
  }

  async function run() {
    setLoading(true)
    setOffline(false)
    try {
      const res = await simulateScenario(form)
      setResult(res)
    } catch {
      setOffline(true)
      setResult(clientSim(form))
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className={s.page}>
      <div className={s.header}>
        <div className={s.eyebrow}>What-If Machine</div>
        <div className={s.title}>Scenario Simulator</div>
        <div className={s.sub}>Set any game state and run 1,000 Monte Carlo simulations instantly.</div>
      </div>

      <div className={s.presets}>
        {SCENARIOS.map(p => (
          <button key={p.id} className={`${s.preset} ${activePreset === p.id ? s.presetActive : ''}`} onClick={() => applyPreset(p)}>
            {p.label}
          </button>
        ))}
      </div>

      <div className={s.builder}>
        <div className={s.builderTitle}>State Builder</div>
        <div className={s.fields}>
          <div className={s.field}>
            <label className={s.label}>Game</label>
            <select className={s.select} value={form.game} onChange={e => set('game', e.target.value)}>
              <option value="valorant">Valorant</option>
              <option value="r6siege">R6 Siege</option>
            </select>
          </div>
          <div className={s.field}>
            <label className={s.label}>Map</label>
            <select className={s.select} value={form.map_name} onChange={e => set('map_name', e.target.value)}>
              {['ascent','bind','haven','split','icebox','fracture','pearl','lotus','sunset','oregon','clubhouse','consulate','villa'].map(m => (
                <option key={m} value={m}>{m.charAt(0).toUpperCase() + m.slice(1)}</option>
              ))}
            </select>
          </div>
          <div className={s.field}>
            <label className={s.label}>Site</label>
            <select className={s.select} value={form.site} onChange={e => set('site', e.target.value)}>
              {['A','B','C','default'].map(s2 => <option key={s2} value={s2}>{s2}</option>)}
            </select>
          </div>
          {[
            ['atk_alive',       'ATK Players Alive',           1,   5],
            ['def_alive',       'DEF Players Alive',           1,   5],
            ['atk_hp',          'ATK Total HP',                5, 750],
            ['def_hp',          'DEF Total HP',                5, 750],
            ['atk_util',        'ATK Utility',                 0,  15],
            ['def_util',        'DEF Utility',                 0,  15],
            ['atk_eco',         'ATK Eco (0=pistol 3=rifle)',  0,   3],
            ['def_eco',         'DEF Eco (0=pistol 3=rifle)',  0,   3],
            ['time_remaining_s','Time Remaining (s)',          1, 180],
          ].map(([key, label, min, max]) => (
            <div key={key} className={s.field}>
              <label className={s.label}>{label}</label>
              <input
                className={s.input} type="number"
                min={min} max={max} value={form[key]}
                onChange={e => set(key, Number(e.target.value))}
              />
            </div>
          ))}
          <div className={s.field}>
            <label className={s.label}>Phase</label>
            <select className={s.select} value={form.phase} onChange={e => set('phase', e.target.value)}>
              {['buy','early','mid','late','plant','defuse'].map(p => (
                <option key={p} value={p}>{p}</option>
              ))}
            </select>
          </div>
          <div className={s.field}>
            <label className={s.label}>Positioning</label>
            <select className={s.select} value={form.positioning} onChange={e => set('positioning', e.target.value)}>
              {POSITIONING_OPTS.map(o => (
                <option key={o.value} value={o.value}>{o.label}</option>
              ))}
            </select>
          </div>
        </div>

        <div className={s.toggleRow}>
          <button
            className={`${s.toggle} ${form.spike_planted ? s.toggleOn : ''}`}
            onClick={() => set('spike_planted', !form.spike_planted)}
          >
            <span className={`${s.toggleDot} ${form.spike_planted ? s.toggleDotOn : ''}`} />
          </button>
          <span className={s.toggleLabel}>Spike Planted</span>

          <button
            className={`${s.toggle} ${form.has_defuser ? s.toggleOn : ''}`}
            onClick={() => set('has_defuser', !form.has_defuser)}
            style={{ marginLeft: 20 }}
          >
            <span className={`${s.toggleDot} ${form.has_defuser ? s.toggleDotOn : ''}`} />
          </button>
          <span className={s.toggleLabel}>DEF Has Defuser</span>
        </div>

        <div className={s.historyRow}>
          <span className={s.historyLabel}>Prior Rounds</span>
          {[0, 1, 2].map(i => {
            const r = form.round_history[i]
            const winner = r?.winner
            function cycle() {
              const next = winner === 'attack' ? 'defense' : winner === 'defense' ? null : 'attack'
              const updated = [...form.round_history]
              if (next === null) updated.splice(i, 1)
              else if (i < updated.length) updated[i] = { winner: next }
              else updated.push({ winner: next })
              set('round_history', updated)
            }
            return (
              <button key={i} onClick={cycle}
                className={`${s.roundSlot} ${winner === 'attack' ? s.roundAtk : winner === 'defense' ? s.roundDef : s.roundEmpty}`}>
                {winner === 'attack' ? 'ATK' : winner === 'defense' ? 'DEF' : '+'}
              </button>
            )
          })}
          <span className={s.historyHint}>Click to cycle ATK / DEF / clear</span>
        </div>

        <button className={s.runBtn} onClick={run} disabled={loading}>
          {loading ? 'Simulating...' : '▶ Run 1,000 Simulations'}
        </button>
      </div>

      {result && (
        <div className={s.result}>
          <div className={s.card}>
            <div className={s.cardTitle}>
              Simulation Result
              {offline && <span className={s.offlineBadge}>Offline Mode</span>}
            </div>
            <OddsGauge
              atkProb={result.blended_prob}
              defProb={100 - result.blended_prob}
              confidence={result.confidence}
              teamAtk="Attackers"
              teamDef="Defenders"
            />
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
            <div className={s.card}>
              <div className={s.cardTitle}>State Summary</div>
              <StatePanel state={{
                atk_alive: form.atk_alive, def_alive: form.def_alive,
                atk_hp: form.atk_hp, def_hp: form.def_hp,
                atk_util: form.atk_util, def_util: form.def_util,
                spike_planted: form.spike_planted,
                has_defuser: form.has_defuser,
                positioning: form.positioning,
              }} />
            </div>
            <DebaterCard debate={result.debate} simProb={result.win_prob_attack} />
          </div>
        </div>
      )}
    </div>
  )
}
