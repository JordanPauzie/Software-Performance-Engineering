import { Outlet } from 'react-router-dom';

export default function Root() {
	return (
		<>
			<header className="siteTop">
				<a className="title" href="/">
					<span className="leiser">leiser</span>
					<span className="chess">chess</span>
				</a>
				<nav className="topMenu">
					<a href="/play">Play</a>
					<a href="/skittles">Skittles</a>
					<a href="/analysis">Analysis</a>
				</nav>
			</header>
			<div id="main">
				<Outlet />
			</div>
		</>
	);
}
