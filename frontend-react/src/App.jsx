import { lazy, Suspense } from 'react'
import { Routes, Route, useLocation } from 'react-router-dom'
import Navbar from './components/Navbar'

const Live     = lazy(() => import('./pages/Live'))
const Simulate = lazy(() => import('./pages/Simulate'))
const History  = lazy(() => import('./pages/History'))
const Overlay  = lazy(() => import('./pages/Overlay'))

function PageLoader() {
  return <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', height: '60vh', color: 'var(--text-muted)', fontSize: 13 }}>Loading...</div>
}

export default function App() {
  const { pathname } = useLocation()
  const isOverlay = pathname === '/overlay'

  return (
    <>
      {!isOverlay && <Navbar />}
      <Suspense fallback={<PageLoader />}>
        <Routes>
          <Route path="/"         element={<Live />} />
          <Route path="/simulate" element={<Simulate />} />
          <Route path="/history"  element={<History />} />
          <Route path="/overlay"  element={<Overlay />} />
        </Routes>
      </Suspense>
    </>
  )
}
