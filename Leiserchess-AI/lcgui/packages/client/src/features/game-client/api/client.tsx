/* eslint-disable max-params */
import {
	makeClientCallback,
	type MoveInfo,
	type ClientToServerEvents,
	type ServerToClientEvents,
	type GameInfo,
	makeClientListener,
	type ClientToServerArgument,
	type GameStatus,
	type GameResult,
} from '@lcgui/shared';
import { io, type Socket } from 'socket.io-client';
import { type Direction, type Board, unabbreviate, fenToBoard, type Color } from '@/types/board';
import {
	getMoveString,
	parseFileRankString,
	parseMoveString,
	type FileRank,
	type Move,
} from '@/types/move';

const socket: Socket<ServerToClientEvents, ClientToServerEvents> = io();

export type ParsedMoveInfo = {
	move: Move;
	endState: Board;
	keyframeBoards: Board[];
	laser: { start: FileRank; steps: Direction[] };
	status: GameStatus;
};

export type ParsedGameInfo = {
	gameId: number;
	moveHistory: ParsedMoveInfo[];
	board: Board;
	whiteTime: number | undefined;
	blackTime: number | undefined;
};

function parseMoveInfo(moveInfo: MoveInfo): ParsedMoveInfo {
	return {
		move: parseMoveString(moveInfo.move),
		endState: fenToBoard(moveInfo.endState)!,
		keyframeBoards: [fenToBoard(moveInfo.keyframe)!],
		laser: {
			start: parseFileRankString(moveInfo.laser.start),
			steps: moveInfo.laser.steps.map((step) => unabbreviate(step)!),
		},
		status: moveInfo.status,
	};
}

function parseGameInfo(gameInfo: GameInfo): ParsedGameInfo {
	return {
		gameId: gameInfo.gameId,
		moveHistory: gameInfo.moveHistory.map((moveInfo) => parseMoveInfo(moveInfo)),
		board: fenToBoard(gameInfo.fen)!,
		whiteTime: gameInfo.whiteTime,
		blackTime: gameInfo.blackTime,
	};
}

type Canceler = () => void;

function makeCancelableCallback<T extends unknown[]>(
	callback: (...argument: T) => void
): [(...argument: T) => void, Canceler] {
	let canceled = false;
	const canceler = () => {
		canceled = true;
	};

	return [
		(...argument) => {
			if (!canceled) {
				callback(...argument);
			}
		},
		canceler,
	];
}

let cachedStartFen: string | undefined;
export function getStartFen(callback: (startFen: string) => void): Canceler {
	if (cachedStartFen === undefined) {
		const [cancelableCallback, canceler] = makeCancelableCallback(callback);
		socket.emit(
			'getStartFen',
			null,
			makeClientCallback('getStartFen', (startFen) => {
				cachedStartFen = startFen;
				cancelableCallback(startFen);
			})
		);
		return canceler;
	}

	callback(cachedStartFen);
	return () => undefined;
}

export function getValidMoves(
	board: Board,
	callback: (validMoveInfos: ParsedMoveInfo[]) => void
): Canceler {
	const [cancelableCallback, canceler] = makeCancelableCallback(callback);
	socket.emit(
		'validMoves',
		board.toFen(),
		makeClientCallback('validMoves', (moveInfos) => {
			cancelableCallback(moveInfos.map((moveInfo) => parseMoveInfo(moveInfo)));
		})
	);
	return canceler;
}

export function getAnalysis(
	board: Board,
	depth: number,
	callback: (pv: Move[], score: number) => void
): Canceler {
	let canceled = false;
	const [cancelableCallback, canceler] = makeCancelableCallback(callback);
	let currentAnalysisId: number | undefined;
	socket.emit(
		'analyze',
		{ fen: board.toFen(), depth },
		makeClientCallback('analyze', (analysisId: number) => {
			if (canceled) {
				socket.emit('cancelAnalysis', analysisId, () => undefined);
			} else {
				currentAnalysisId = analysisId;
			}
		})
	);
	const clientListener = makeClientListener('analysisReported', ({ pv, score, analysisId }) => {
		if (analysisId === currentAnalysisId) {
			cancelableCallback(
				pv.map((moveString) => parseMoveString(moveString)),
				score
			);
		}
	});
	socket.on('analysisReported', clientListener);
	return () => {
		canceler();
		canceled = true;
		socket.off('analysisReported', clientListener);
		if (currentAnalysisId !== undefined) {
			socket.emit('cancelAnalysis', currentAnalysisId, () => undefined);
		}
	};
}

export type GameControl = ClientToServerArgument<'startGame'>['control'];

export function startGame(
	control: GameControl,
	playerColor: Color,
	initialize: (parsedGameInfo: ParsedGameInfo) => void,
	processMove: (parsedGameInfo: ParsedGameInfo) => void,
	processGameResult: (gameResult: GameResult, parsedGameInfo: ParsedGameInfo) => void
): Canceler {
	let canceled = false;
	const [safeInitialize, cancelInitialize] = makeCancelableCallback(initialize);
	const [safeProcessMove, cancelProcessMove] = makeCancelableCallback(processMove);
	const [safeProcessGameResult, cancelProcessGameResult] =
		makeCancelableCallback(processGameResult);

	let currentGameId: number | undefined;
	const moveListener = makeClientListener('moveMade', (gameInfo) => {
		if (gameInfo.gameId === currentGameId) {
			safeProcessMove(parseGameInfo(gameInfo));
		}
	});
	const gameResultListener = makeClientListener('gameEnded', ({ gameInfo, gameResult }) => {
		if (gameInfo.gameId === currentGameId) {
			safeProcessGameResult(gameResult, parseGameInfo(gameInfo));
		}
	});
	socket.on('moveMade', moveListener);
	socket.on('gameEnded', gameResultListener);
	socket.emit(
		'startGame',
		{
			control,
			userPlaysWhite: playerColor === 'white',
		},
		makeClientCallback('startGame', (gameInfo: GameInfo) => {
			if (canceled) {
				socket.emit('cancelGame', gameInfo.gameId, () => undefined);
			} else {
				currentGameId = gameInfo.gameId;
				safeInitialize(parseGameInfo(gameInfo));
			}
		})
	);
	return () => {
		cancelInitialize();
		cancelProcessMove();
		cancelProcessGameResult();
		canceled = true;
		socket.off('moveMade', moveListener);
		socket.off('gameEnded', gameResultListener);
		if (currentGameId !== undefined) {
			socket.emit('cancelGame', currentGameId, () => undefined);
		}
	};
}

export function submitMove(
	gameId: number,
	move: Move,
	callback: (parsedGameInfo: ParsedGameInfo) => void
): Canceler {
	const [cancelableCallback, canceler] = makeCancelableCallback(callback);
	socket.emit(
		'move',
		{ move: getMoveString(move), gameId },
		makeClientCallback('move', (gameInfo: GameInfo) => {
			cancelableCallback(parseGameInfo(gameInfo));
		})
	);
	return canceler;
}
