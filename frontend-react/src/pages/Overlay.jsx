import { useState, useEffect, useRef } from 'react'
import { MOCK_MATCH, MOCK_TICKS } from '../data/mockMatch'
import s from './Overlay.module.css'

const TICK_MS  = 1000
const WS_URL   = `ws://${window.location.host}/api/ws/${MOCK_MATCH.match_id}`

// Make body transparent for OBS window capture
const transparentStyle = document.createElement('style')
transparentStyle.textContent = 'body, #root { background: transparent !important; }'

const POSITIONING_ZONES = {
  execute:     ['a-site',  'ct'],
  approach:    ['a-main',  'ct'],
  even:        ['default', 'default'],
  def_forward: ['spawn',   'mid'],
}

function tickToStateIn(tick) {
  const ss  = tick.state_summary
  const atk = ss.atk_alive
  const def = ss.def_alive
  const [atkZone, defZone] = POSITIONING_ZONES[ss.positioning] || ['default', 'default']
  const players = []
  for (let i = 0; i < atk; i++) {
    players.push({ id: `atk-${i}`, team: 'attack', alive: true, hp: Math.round(ss.atk_hp / Math.max(atk, 1)), shield: 50, has_spike: i === 0, utilities: Array(Math.round(ss.atk_util / Math.max(atk, 1))).fill('smoke'), weapon: 'vandal', weapon_tier: 2, position: atkZone })
  }
  for (let i = 0; i < def; i++) {
    players.push({ id: `def-${i}`, team: 'defense', alive: true, hp: Math.round(ss.def_hp / Math.max(def, 1)), shield: 50, has_defuser: i === 0 && !!ss.has_defuser, utilities: Array(Math.round(ss.def_util / Math.max(def, 1))).fill('smoke'), weapon: 'vandal', weapon_tier: 2, position: defZone })
  }
  return {
    game: MOCK_MATCH.game, match_id: MOCK_MATCH.match_id,
    round_number: MOCK_MATCH.round_number, phase: tick.phase,
    time_remaining_s: tick.time_remaining_s, spike_planted: ss.spike_planted,
    spike_time_remaining_s: ss.spike_planted ? 35 : null,
    team_attack: MOCK_MATCH.team_attack, team_defense: MOCK_MATCH.team_defense,
    map_name: MOCK_MATCH.map_name, site: MOCK_MATCH.site,
    score_attack: MOCK_MATCH.score_attack, score_defense: MOCK_MATCH.score_defense,
    round_history: MOCK_MATCH.round_history, players,
  }
}

export default function Overlay() {
  const [tickIdx,     setTickIdx]     = useState(0)
  const [playing,     setPlaying]     = useState(true)
  const [wsConnected, setWsConnected] = useState(false)
  const [liveData,    setLiveData]    = useState(null)
  const intervalRef = useRef(null)
  const wsRef       = useRef(null)

  const mockTick = MOCK_TICKS[tickIdx]
  const current  = liveData || mockTick
  const match    = MOCK_MATCH
  const atk      = current.blended_prob
  const def      = 100 - atk
  const topFactor = current.factors?.[0]

  useEffect(() => {
    document.head.appendChild(transparentStyle)
    return () => { document.head.removeChild(transparentStyle) }
  }, [])

  useEffect(() => {
    let ws
    try {
      ws = new WebSocket(WS_URL)
      ws.onopen    = () => setWsConnected(true)
      ws.onclose   = () => setWsConnected(false)
      ws.onerror   = () => setWsConnected(false)
      ws.onmessage = (e) => {
        const data = JSON.parse(e.data)
        if (data.type !== 'heartbeat') setLiveData(data)
      }
      wsRef.current = ws
    } catch { /* no backend — stay on mock */ }
    return () => ws?.close()
  }, [])

  useEffect(() => {
    if (!playing) { clearInterval(intervalRef.current); return }
    intervalRef.current = setInterval(() => {
      setTickIdx(i => i >= MOCK_TICKS.length - 1 ? 0 : i + 1)
    }, TICK_MS)
    return () => clearInterval(intervalRef.current)
  }, [playing])

  useEffect(() => {
    if (!wsConnected || !wsRef.current) return
    wsRef.current.send(JSON.stringify(tickToStateIn(mockTick)))
  }, [tickIdx, wsConnected])

  return (
    <div className={s.overlay} onClick={() => setPlaying(p => !p)}>
      <div className={s.widget}>
        <span className={`${s.liveDot} ${wsConnected ? s.liveDotWs : ''}`} />

        <div className={s.teamBlock}>
          <span className={`${s.teamName} ${s.teamAtk}`}>{match.team_attack}</span>
          <span className={`${s.teamProb} ${atk > 55 ? s.probAtk : s.probEven}`}>{atk.toFixed(1)}%</span>
          <span className={s.teamRole}>ATTACK</span>
        </div>

        <div className={s.center}>
          <div className={s.bar}>
            <div className={s.barFill} style={{ width: `${atk}%` }} />
          </div>
          <div className={s.meta}>
            <div className={s.metaLeft}>
              <span className={s.metaTag}>{mockTick.phase}</span>
              <span className={s.metaTag}>{mockTick.time_remaining_s}s</span>
              {mockTick.state_summary?.spike_planted && (
                <span className={`${s.metaTag} ${s.tagPlant}`}>SPIKE</span>
              )}
              {wsConnected && <span className={`${s.metaTag} ${s.tagLive}`}>LIVE</span>}
            </div>
            <span className={s.confidence}>conf {(current.confidence * 100).toFixed(0)}%</span>
          </div>
          {topFactor && (
            <div className={s.factor}>
              <span className={topFactor.delta >= 0 ? s.factorAccent : s.factorRed}>
                {topFactor.delta >= 0 ? '+' : ''}{topFactor.delta?.toFixed(1)}%
              </span>
              {' '}{topFactor.label}
            </div>
          )}
        </div>

        <div className={s.teamBlock}>
          <span className={`${s.teamName} ${s.teamDef}`}>{match.team_defense}</span>
          <span className={`${s.teamProb} ${def > 55 ? s.probDef : s.probEven}`}>{def.toFixed(1)}%</span>
          <span className={s.teamRole}>DEFENSE</span>
        </div>
      </div>
    </div>
  )
}
