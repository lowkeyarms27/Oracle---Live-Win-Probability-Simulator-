const BASE = '/api'

export async function predict(stateIn) {
  const r = await fetch(`${BASE}/predict`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(stateIn),
  })
  if (!r.ok) throw new Error(await r.text())
  return r.json()
}

export async function simulateScenario(params) {
  const payload = { ...params, round_history: JSON.stringify(params.round_history || []) }
  const qs = new URLSearchParams(payload).toString()
  const r = await fetch(`${BASE}/simulate/scenario?${qs}`)
  if (!r.ok) throw new Error(await r.text())
  return r.json()
}

export async function getTimeline(matchId) {
  const r = await fetch(`${BASE}/timeline/${matchId}`)
  if (!r.ok) throw new Error(await r.text())
  return r.json()
}
