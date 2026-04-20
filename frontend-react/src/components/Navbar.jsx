import { NavLink } from 'react-router-dom'
import s from './Navbar.module.css'

export default function Navbar({ live }) {
  return (
    <nav className={s.nav}>
      <div className={s.logo}>
        <span className={s.logoIcon}>◆</span> Oracle
      </div>
      <NavLink to="/"         end className={({ isActive }) => `${s.link} ${isActive ? s.linkActive : ''}`}>Live</NavLink>
      <NavLink to="/simulate"    className={({ isActive }) => `${s.link} ${isActive ? s.linkActive : ''}`}>Simulate</NavLink>
      <NavLink to="/history"     className={({ isActive }) => `${s.link} ${isActive ? s.linkActive : ''}`}>History</NavLink>
      {live && (
        <div className={s.live}>
          <span className={s.liveDot} /> LIVE
        </div>
      )}
    </nav>
  )
}
