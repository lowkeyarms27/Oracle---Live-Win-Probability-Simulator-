import { useState, useEffect, useRef } from 'react'
import { useNavigate } from 'react-router-dom'
import { MOCK_MATCH, MOCK_TICKS } from '../data/mockMatch'
import OddsGauge from '../components/OddsGauge'
import TimelineChart from '../components/TimelineChart'
import DebaterCard from '../components/DebaterCard'
import StatePanel from '../components/StatePanel'
import RoundSummary from '../components/RoundSummary'
import s from './Live.module.css'

const TICK_MS = 1000
const WS_URL = `ws://${window.location.host}/api/ws/${MOCK_MATCH.match_id}`

const POSITIONING_ZONES = {
  execute:     ['a-site',  'ct'],
  approach:    ['a-main',  'ct'],
  even:        ['default', 'default'],
  def_forward: ['spawn',   'mid'],
}

function tickToStateIn(tick) {
  const ss = tick.state_summary
  const atk = ss.atk_alive
  const def = ss.def_alive
  const [atkZone, defZone] = POSITIONING_ZONES[ss.positioning] || ['default', 'default']
  const players = []
  for (let i = 0; i < atk; i++) {
    players.push({
      id: `atk-${i}`, team: 'attack', alive: true,
      hp: Math.round(ss.atk_hp / Math.max(atk, 1)), shield: 50,
      has_spike: i === 0,
      utilities: Array(Math.round(ss.atk_util / Math.max(atk, 1))).fill('smoke'),
      weapon: 'vandal', weapon_tier: 2, position: atkZone,
    })
  }
  for (let i = 0; i < def; i++) {
    players.push({
      id: `def-${i}`, team: 'defense', alive: true,
      hp: Math.round(ss.def_hp / Math.max(def, 1)), shield: 50,
      has_defuser: i === 0 && !!ss.has_defuser,
      utilities: Array(Math.round(ss.def_util / Math.max(def, 1))).fill('smoke'),
      weapon: 'vandal', weapon_tier: 2, position: defZone,
    })
  }
  return {
    game: MOCK_MATCH.game,
    match_id: MOCK_MATCH.match_id,
    round_number: MOCK_MATCH.round_number,
    phase: tick.phase,
    time_remaining_s: tick.time_remaining_s,
    spike_planted: ss.spike_planted,
    spike_time_remaining_s: ss.spike_planted ? 35 : null,
    team_attack: MOCK_MATCH.team_attack,
    team_defense: MOCK_MATCH.team_defense,
    map_name: MOCK_MATCH.map_name,
    site: MOCK_MATCH.site,
    score_attack: MOCK_MATCH.score_attack,
    score_defense: MOCK_MATCH.score_defense,
    round_history: MOCK_MATCH.round_history,
    players,
  }
}

export default function Live() {
  const [tickIdx,     setTickIdx]     = useState(0)
  const [playing,     setPlaying]     = useState(false)
  const [ticks,       setTicks]       = useState([MOCK_TICKS[0]])
  const [done,        setDone]        = useState(false)
  const [wsMode,      setWsMode]      = useState(false)
  const [wsConnected, setWsConnected] = useState(false)
  const [wsResult,    setWsResult]    = useState(null)
  const intervalRef = useRef(null)
  const wsRef       = useRef(null)
  const navigate    = useNavigate()

  const current = ticks[ticks.length - 1]
  const match   = MOCK_MATCH
  const display = wsMode && wsResult
    ? { ...current, blended_prob: wsResult.blended_prob, win_prob_attack: wsResult.win_prob_attack, confidence: wsResult.confidence, factors: wsResult.factors, debate: wsResult.debate }
    : current

  function connectWs() {
    try {
      const ws = new WebSocket(WS_URL)
      ws.onopen  = () => setWsConnected(true)
      ws.onclose = () => { setWsConnected(false); setWsMode(false); setWsResult(null) }
      ws.onerror = () => { setWsConnected(false); setWsMode(false) }
      ws.onmessage = (e) => {
        const data = JSON.parse(e.data)
        if (data.type !== 'heartbeat') setWsResult(data)
      }
      wsRef.current = ws
      setWsMode(true)
    } catch {
      setWsMode(false)
    }
  }

  function disconnectWs() {
    wsRef.current?.close()
    wsRef.current = null
    setWsConnected(false)
    setWsMode(false)
    setWsResult(null)
  }

  function toggleWs() {
    if (wsMode) disconnectWs()
    else connectWs()
  }

  function play() {
    if (done) { reset(); return }
    setPlaying(true)
  }

  function pause() { setPlaying(false) }

  function reset() {
    setPlaying(false); setTickIdx(0); setTicks([MOCK_TICKS[0]]); setDone(false); setWsResult(null)
  }

  useEffect(() => {
    if (!playing) { clearInterval(intervalRef.current); return }
    intervalRef.current = setInterval(() => {
      setTickIdx(i => {
        const next = i + 1
        if (next >= MOCK_TICKS.length) {
          setPlaying(false); setDone(true); return i
        }
        setTicks(prev => [...prev, MOCK_TICKS[next]])
        return next
      })
    }, TICK_MS)
    return () => clearInterval(intervalRef.current)
  }, [playing])

  useEffect(() => {
    if (!wsMode || !wsConnected || !wsRef.current) return
    const stateIn = tickToStateIn(MOCK_TICKS[tickIdx])
    wsRef.current.send(JSON.stringify(stateIn))
  }, [tickIdx, wsMode, wsConnected])

  useEffect(() => {
    return () => { wsRef.current?.close() }
  }, [])

  const phaseClass = current.phase === 'plant' ? s.phasePlant
    : current.phase === 'defuse' ? s.phaseDefuse : s.phase

  return (
    <div className={s.page}>
      <div className={s.matchHeader}>
        <div className={s.matchTeams}>
          <span className={s.atk}>{match.team_attack}</span>
          <span className={s.vs}>vs</span>
          <span className={s.def}>{match.team_defense}</span>
        </div>
        <div className={s.matchMeta}>
          <span className={s.metaTag}>{match.game}</span>
          <span className={s.metaTag}>{match.map} · {match.site}-site</span>
          <span className={s.metaTag}>Round {match.round_number}</span>
          <span className={s.metaTag}>{match.score_attack} – {match.score_defense}</span>
          <span className={`${s.phase} ${phaseClass}`}>{current.phase}</span>
        </div>
        <div className={s.timer}>{current.time_remaining_s}s</div>
      </div>

      <div className={s.controls}>
        {!playing
          ? <button className={`${s.btn} ${s.btnPrimary}`} onClick={play}>{done ? '↺ Replay' : '▶ Play Demo'}</button>
          : <button className={s.btn} onClick={pause}>⏸ Pause</button>
        }
        <button className={`${s.btn} ${s.btnDanger}`} onClick={reset}>↺ Reset</button>
        <button className={s.btn} onClick={() => navigate('/overlay')} title="Open broadcast overlay">
          ⬜ Overlay
        </button>
        <button
          className={`${s.btn} ${wsMode ? s.btnWsOn : s.btnWsOff}`}
          onClick={toggleWs}
          title={wsMode ? 'Disconnect from backend' : 'Connect to live backend'}
        >
          {wsMode
            ? wsConnected ? '● Live Backend' : '◌ Connecting...'
            : '○ Connect Backend'}
        </button>
        <span style={{ fontSize: 11, color: 'var(--text-muted)', alignSelf: 'center' }}>
          Tick {tickIdx + 1}/{MOCK_TICKS.length} · t={tickIdx * 5}s
        </span>
        {match.round_history?.length > 0 && (
          <span style={{ fontSize: 11, color: 'var(--text-muted)', alignSelf: 'center', marginLeft: 'auto' }}>
            History: {match.round_history.map(r => r.winner === 'attack' ? 'W' : 'L').join('-')}
          </span>
        )}
      </div>

      <div className={s.grid}>
        <div className={s.card}>
          <div className={s.cardTitle}>
            Live Win Probability
            {wsMode && wsConnected && <span className={s.livePill}>LIVE</span>}
          </div>
          <OddsGauge
            atkProb={display.blended_prob}
            defProb={100 - display.blended_prob}
            confidence={display.confidence}
            teamAtk={match.team_attack}
            teamDef={match.team_defense}
          />
        </div>
        <div className={s.card}>
          <TimelineChart ticks={ticks} />
          <div style={{ marginTop: 20 }}>
            <StatePanel state={current.state_summary} />
          </div>
        </div>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1.4fr', gap: 20, marginBottom: 20 }}>
        <div className={s.card}>
          <div className={s.cardTitle}>Key Factors</div>
          <div className={s.factors}>
            {(display.factors || []).map((f, i) => (
              <div key={i} className={s.factor}>
                <span className={s.factorLabel}>{f.label}</span>
                <span className={f.delta >= 0 ? s.factorDeltaPos : s.factorDeltaNeg}>
                  {f.delta >= 0 ? '+' : ''}{f.delta?.toFixed(1)}%
                </span>
              </div>
            ))}
            {!display.factors?.length && (
              <span style={{ color: 'var(--text-muted)', fontSize: 12 }}>No dominant factors yet.</span>
            )}
          </div>
        </div>
        <DebaterCard debate={display.debate} simProb={display.win_prob_attack} />
      </div>

      {done && (
        <RoundSummary
          ticks={ticks}
          teamAtk={match.team_attack}
          teamDef={match.team_defense}
        />
      )}
    </div>
  )
}
