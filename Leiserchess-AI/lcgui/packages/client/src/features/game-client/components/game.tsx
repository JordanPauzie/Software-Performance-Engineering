import assert from 'node:assert';
import { GameClock, type GameStatus, type GameResult } from '@lcgui/shared';
import { useCallback, useEffect, useState } from 'react';
import { match } from 'ts-pattern';
import {
	type ParsedGameInfo,
	startGame,
	type GameControl,
	type ParsedMoveInfo,
	submitMove,
} from '../api/client';
import GameBoard from './game-board';
import Timer from './timer';
import { type Color, type Board } from '@/types/board';
import { type AtLeastOne } from '@/utils';
import MoveHistory from '@/features/move-history/components/move-history';
import { type Move } from '@/types/move';
import PgnInput from '@/features/analysis/components/pgn-input';

type GameControlState =
	| {
			kind: 'time';
			gameClock: GameClock;
	  }
	| { kind: 'depth'; depth: number };

type GameState = {
	gameControls: GameControlState;
	startBoard: Board;
	keyframeBoards: Board[];
	laserFrame: number;
	moveHistory: ParsedMoveInfo[];
	currentIndex: number;
	gameId: number;
};

type Properties = {
	readonly gameControl: GameControl;
	readonly playerColor: Color;
};

export default function Game({ gameControl, playerColor }: Properties) {
	const [gameState, setGameState] = useState<GameState | undefined>(undefined);
	const [gameResult, setGameResult] = useState<GameStatus>('ongoing');
	const [perspective, setPerspective] = useState<Color>(playerColor);

	const getCurrentBoard = useCallback((gameState: GameState) => {
		if (gameState.currentIndex === 0) return gameState.startBoard;
		return gameState.moveHistory[gameState.currentIndex - 1]!.endState;
	}, []);
	useEffect(() => {
		const updateGameControl = (parsedGameInfo: ParsedGameInfo): GameControlState => {
			if (gameControl.kind === 'time') {
				assert(parsedGameInfo.whiteTime !== undefined && parsedGameInfo.blackTime !== undefined);
				return {
					kind: 'time',
					gameClock: new GameClock(
						parsedGameInfo.whiteTime,
						parsedGameInfo.blackTime,
						gameControl.whiteIncrement,
						gameControl.blackIncrement,
						parsedGameInfo.moveHistory.length % 2 === 0 ? 'white' : 'black',
						() => undefined
					),
				};
			}

			return { kind: 'depth', depth: gameControl.depth };
		};

		const initialize = (parsedGameInfo: ParsedGameInfo) => {
			setGameState({
				gameControls: updateGameControl(parsedGameInfo),
				startBoard: parsedGameInfo.board,
				keyframeBoards: [],
				laserFrame: 0,
				moveHistory: [],
				currentIndex: 0,
				gameId: parsedGameInfo.gameId,
			});
		};

		const processMove = (parsedGameInfo: ParsedGameInfo) => {
			setGameState((gameState) => {
				if (gameState === undefined) return undefined;
				let keyframeBoards = [getCurrentBoard(gameState)];
				let laserFrame = 1;
				if (gameState.currentIndex === parsedGameInfo.moveHistory.length - 1) {
					const lastMove = parsedGameInfo.moveHistory.at(-1);
					assert(lastMove !== undefined);
					keyframeBoards = [getCurrentBoard(gameState), ...lastMove.keyframeBoards];
					laserFrame = lastMove.keyframeBoards.length;
				}

				if (parsedGameInfo.moveHistory.length === gameState.moveHistory.length) {
					return { ...gameState, gameControls: updateGameControl(parsedGameInfo) };
				}

				return {
					gameControls: updateGameControl(parsedGameInfo),
					keyframeBoards,
					laserFrame,
					startBoard: gameState.startBoard,
					moveHistory: parsedGameInfo.moveHistory,
					currentIndex: parsedGameInfo.moveHistory.length,
					gameId: parsedGameInfo.gameId,
				};
			});
		};

		const processGameResult = (gameResult: GameResult, parsedGameInfo: ParsedGameInfo) => {
			setGameState((gameState) =>
				gameState === undefined
					? undefined
					: {
							...gameState,
							gameControls: updateGameControl(parsedGameInfo),
						}
			);
			setGameResult(gameResult);
		};

		return startGame(gameControl, playerColor, initialize, processMove, processGameResult);
	}, [gameControl, playerColor, getCurrentBoard]);

	const [whiteTimeLeft, setWhiteTimeLeft] = useState<number | undefined>(undefined);
	const [blackTimeLeft, setBlackTimeLeft] = useState<number | undefined>(undefined);
	const refreshTime = useCallback(() => {
		if (
			gameState !== undefined &&
			gameResult === 'ongoing' &&
			gameState.gameControls.kind === 'time'
		) {
			const [whiteTime, blackTime] = gameState.gameControls.gameClock.getTimeLeft();
			setWhiteTimeLeft(whiteTime);
			setBlackTimeLeft(blackTime);
		}
	}, [gameState, gameResult]);
	useEffect(() => {
		refreshTime();
		const interval = setInterval(refreshTime, 1000);
		return () => {
			clearInterval(interval);
		};
	}, [refreshTime]);
	const transitions: AtLeastOne<Array<[number, Move]>> =
		gameState === undefined
			? [[] as Array<[number, Move]>]
			: (gameState.moveHistory
					.flatMap((parsedMoveInfo, index) =>
						parsedMoveInfo === undefined
							? []
							: [[[index + 1, parsedMoveInfo.move] as [number, Move]]]
					)
					.concat([[]]) as AtLeastOne<Array<[number, Move]>>);

	return (
		<div className="game">
			{gameState === undefined ? (
				<div>Loading...</div>
			) : (
				<>
					<div className="gameBody">
						<GameBoard
							board={getCurrentBoard(gameState)}
							keyframeBoards={gameState.keyframeBoards}
							laserFrame={gameState.laserFrame}
							lastMove={gameState.moveHistory[gameState.currentIndex - 1]}
							selectMove={(parsedMoveInfo: ParsedMoveInfo) => {
								if (gameState.gameControls.kind === 'time') {
									gameState.gameControls.gameClock.toggleSide();
								}

								refreshTime();

								setGameState((gameState) => {
									if (gameState === undefined) return undefined;

									return {
										gameControls: gameState.gameControls,
										startBoard: gameState.startBoard,
										moveHistory: [...gameState.moveHistory, parsedMoveInfo],
										keyframeBoards: [getCurrentBoard(gameState), ...parsedMoveInfo.keyframeBoards],
										laserFrame: parsedMoveInfo.keyframeBoards.length,
										currentIndex: gameState.moveHistory.length + 1,
										gameId: gameState.gameId,
									};
								});
								submitMove(gameState.gameId, parsedMoveInfo.move, () => undefined);
							}}
							playableColors={
								gameState.currentIndex === gameState.moveHistory.length ? [playerColor] : []
							}
							defaultBoardPerspective={playerColor}
							setPerspective={setPerspective}
						/>
						<div className="gameSidebar">
							{whiteTimeLeft !== undefined && blackTimeLeft !== undefined ? (
								<Timer
									isActive={
										(gameState.moveHistory.length % 2 === 0 ? 'black' : 'white') === perspective &&
										gameResult === 'ongoing'
									}
									timeLeft={perspective === 'white' ? blackTimeLeft : whiteTimeLeft}
								/>
							) : null}
							<MoveHistory
								transitions={transitions}
								parents={[undefined, ...gameState.moveHistory.flatMap((_, index) => index)]}
								selectedNode={gameState.currentIndex}
								selectNode={(nodeIndex) => {
									const [keyframeBoards, laserFrame] = (() => {
										if (nodeIndex === gameState.currentIndex - 1) {
											const keyframes = [
												getCurrentBoard(gameState),
												...(gameState.moveHistory[nodeIndex]?.keyframeBoards.slice().reverse() ??
													[]),
											];
											return [keyframes, keyframes.length];
										}

										if (nodeIndex === gameState.currentIndex + 1) {
											const keyframes = [
												getCurrentBoard(gameState),
												...(gameState.moveHistory[gameState.currentIndex]?.keyframeBoards ?? []),
											];
											return [keyframes, keyframes.length - 1];
										}

										return [[getCurrentBoard(gameState)], 1];
									})();
									setGameState((gameState) =>
										gameState === undefined
											? undefined
											: {
													...gameState,
													currentIndex: nodeIndex,
													keyframeBoards,
													laserFrame,
												}
									);
								}}
							/>
							{gameResult === 'ongoing' ? null : (
								<div className={`gameResult ${gameResult}`}>{`${match(gameResult)
									.with('wwin', () => 'Tangerine wins!')
									.with('bwin', () => 'Lavender wins!')
									.with('draw', () => 'Draw')
									.exhaustive()}`}</div>
							)}
							{whiteTimeLeft !== undefined && blackTimeLeft !== undefined ? (
								<Timer
									isActive={
										(gameState.moveHistory.length % 2 === 0 ? 'white' : 'black') === perspective &&
										gameResult === 'ongoing'
									}
									timeLeft={perspective === 'white' ? whiteTimeLeft : blackTimeLeft}
								/>
							) : null}
						</div>
					</div>
					<div className="gameFooter">
						<PgnInput transitions={transitions} setTransitions={undefined} />
					</div>
				</>
			)}
		</div>
	);
}
