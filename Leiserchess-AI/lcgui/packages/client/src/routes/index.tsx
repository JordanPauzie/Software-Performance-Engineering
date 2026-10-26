import { createBrowserRouter } from 'react-router-dom';
import Root from '@/routes/root';
import GamePage from '@/features/game-client/routes';
import Skittles from '@/features/skittles/routes';
import Analysis from '@/features/analysis/routes';

export const router = createBrowserRouter([
	{
		path: '/',
		element: <Root />,
		children: [
			{ index: true, element: <GamePage /> },
			{ path: 'skittles', element: <Skittles /> },
			{ path: 'analysis', element: <Analysis /> },
			{ path: 'play', element: <GamePage /> },
		],
	},
]);
