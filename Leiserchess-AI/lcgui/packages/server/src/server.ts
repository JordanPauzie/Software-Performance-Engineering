/* eslint-disable unicorn/prefer-top-level-await */
import { Server, type Socket } from 'socket.io';
import { Mutex } from 'async-mutex';
import {
	makeServerListener,
	type MoveInfo,
	type ClientToServerEvents,
	type ServerToClientEvents,
	type GameInfo,
	type GameResult,
} from '@lcgui/shared';
import { RefereeBinary } from './referee';
import { GameState } from './game-state';
import { Deferred } from './deferred';

(async () => {
	const referee = new RefereeBinary();
	const refereeLock = new Mutex();

	const io = new Server<ClientToServerEvents, ServerToClientEvents>(3000);
	const games = new Map<number, GameState>();
	const listeners = new Map<number, Array<Socket<ClientToServerEvents, ServerToClientEvents>>>();
	const listenerToGameId = new Map<string, number[]>();
	const gameCancelHooks = new Map<number, Deferred<void>>();
	const analysisCancelHooks = new Map<number, Deferred<void>>();
	const analysesBySocket = new Map<string, number[]>();
	let gameCount = 0;
	let analysisCount = 0;
	referee.writeLine('position startpos\nprintfen\n');
	const output = await referee.readOutputLine();
	const startFen = output.trim();
	async function makeMove(gameState: GameState, move: string): Promise<GameInfo | undefined> {
		if (gameState !== undefined) {
			for await (const moveInfo of referee.readMoveInfo(gameState.getFen())) {
				if (moveInfo.move === move) {
					if (gameState.addMoveToState(moveInfo, moveInfo.endState)) {
						const gameInfo = gameState.getGameInfo();
						return gameInfo;
					}

					break;
				}
			}
		}

		return undefined;
	}

	function declareResult(gameState: GameState, result: GameResult) {
		for (const listener of listeners.get(gameState.gameId) ?? []) {
			listener.emit('gameEnded', {
				gameInfo: gameState.getGameInfo(),
				gameResult: result,
			});
			const remainingGames = (listenerToGameId.get(listener.id) ?? []).filter(
				(otherGameId) => otherGameId !== gameState.gameId
			);
			if (remainingGames.length > 0) {
				listenerToGameId.set(listener.id, remainingGames);
			} else {
				listenerToGameId.delete(listener.id);
			}
		}

		listeners.delete(gameState.gameId);
		games.delete(gameState.gameId);
	}

	async function callCpuIfNeeded(gameState: GameState): Promise<void> {
		if (gameState.isCpuTurn()) {
			const cancelHook = gameCancelHooks.get(gameState.gameId);
			if (cancelHook === undefined) {
				return;
			}

			const [promiseIndex, cpuMove] = await Promise.any(
				[
					cancelHook.promise,
					referee.getBestMove(gameState.getFen(), gameState.getCpuSearchCommand()),
				].map(async (promise, index) => [index, await promise] as const)
			);
			if (promiseIndex === 0) {
				declareResult(gameState, gameState.userPlaysWhite ? 'bwin' : 'wwin');
				referee.restartBinary();
			} else if (cpuMove === undefined) {
				declareResult(gameState, gameState.userPlaysWhite ? 'wwin' : 'bwin');
			} else {
				const [promiseIndex, moveResult] = await Promise.any(
					[cancelHook.promise, makeMove(gameState, cpuMove)].map(
						async (promise, index) => [index, await promise] as const
					)
				);
				if (promiseIndex === 0) {
					declareResult(gameState, gameState.userPlaysWhite ? 'bwin' : 'wwin');
					referee.restartBinary();
				} else if (moveResult === undefined) {
					declareResult(gameState, gameState.userPlaysWhite ? 'wwin' : 'bwin');
				} else {
					const gameInfo = moveResult;
					for (const listener of listeners.get(gameState.gameId) ?? []) {
						listener.emit('moveMade', gameInfo);
					}

					const status = gameState.getStatus();
					if (status !== 'ongoing') {
						declareResult(gameState, status);
					}
				}
			}
		}
	}

	console.log('hello world');
	io.on('connection', async (socket) => {
		console.log(`${socket.id} connected`);
		listenerToGameId.set(socket.id, []);
		socket.on(
			'startGame',
			makeServerListener('startGame', async (argument, callback) => {
				return refereeLock.runExclusive(async () => {
					const newState = new GameState(startFen, gameCount, argument, (color) => {
						declareResult(newState, color === 'white' ? 'bwin' : 'wwin');
					});
					games.set(gameCount, newState);
					listeners.set(gameCount, [socket]);
					listenerToGameId.get(socket.id)?.push(gameCount);
					gameCancelHooks.set(gameCount, new Deferred<void>());
					callback(newState.getGameInfo());
					gameCount += 1;
					await callCpuIfNeeded(newState);
				});
			})
		);
		socket.on(
			'cancelGame',
			makeServerListener('cancelGame', async (argument, callback) => {
				const cancelHook = gameCancelHooks.get(argument);
				if (cancelHook === undefined) {
					callback(false);
				} else {
					callback(true);
					cancelHook.resolve();
				}
			})
		);
		socket.on(
			'move',
			makeServerListener('move', async ({ move, gameId }, callback) => {
				return refereeLock.runExclusive(async () => {
					const gameState = games.get(gameId);
					if (gameState !== undefined && !gameState.isCpuTurn()) {
						const moveResult = await makeMove(gameState, move);
						if (moveResult !== undefined) {
							for (const listener of listeners.get(gameId) ?? []) {
								listener.emit('moveMade', moveResult);
							}

							callback(moveResult);
							const status = gameState.getStatus();
							if (status !== 'ongoing') {
								declareResult(gameState, status);
							}

							await callCpuIfNeeded(gameState);
						}
					}
				});
			})
		);
		socket.on(
			'validMoves',
			makeServerListener('validMoves', async (fen, callback) => {
				const result: MoveInfo[] = [];
				await refereeLock.runExclusive(async () => {
					for await (const moveInfo of referee.readMoveInfo(fen)) {
						result.push(moveInfo);
					}
				});
				callback(result);
			})
		);
		socket.on(
			'listenGame',
			makeServerListener('listenGame', async (gameId, callback) => {
				const existingListeners = listeners.get(gameId);
				const gameState = games.get(gameId);
				if (existingListeners !== undefined && gameState !== undefined) {
					existingListeners.push(socket);
					listenerToGameId.get(socket.id)?.push(gameId);
					callback(gameState.getGameInfo());
				}
			})
		);
		socket.on(
			'analyze',
			makeServerListener('analyze', async ({ fen, depth }, callback) => {
				const analysisId = analysisCount++;
				analysesBySocket.set(
					socket.id,
					(analysesBySocket.get(socket.id) ?? []).concat([analysisId])
				);

				const cancelHook = new Deferred<void>();
				analysisCancelHooks.set(analysisId, cancelHook);
				callback(analysisId);
				await refereeLock.runExclusive(async () => {
					if (fen.includes('\n') || fen.includes('\r')) return;
					referee.writeLine(`fenvalid ${fen}\n`);
					const validOut = await Promise.any([cancelHook.promise, referee.readOutputLine()]);
					if (validOut !== undefined && validOut.trim() === 'valid') {
						referee.writeLine(`position fen ${fen}\ngo depth ${depth}\n`);
						while (true) {
							const nextLine = await Promise.any([cancelHook.promise, referee.readOutputLine()]); // eslint-disable-line no-await-in-loop
							if (nextLine === undefined) {
								analysisCancelHooks.delete(analysisId);
								referee.restartBinary();
								break;
							} else if (nextLine.startsWith('info score')) {
								const tokens = nextLine.trim().split(' ');
								const score = Number.parseInt(tokens[3]!, 10);
								socket.emit('analysisReported', { pv: tokens.slice(5), score, analysisId });
							} else if (nextLine.startsWith('bestmove')) {
								break;
							}
						}
					}
				});

				analysesBySocket.set(
					socket.id,
					(analysesBySocket.get(socket.id) ?? []).filter((otherId) => otherId !== analysisId)
				);
			})
		);
		socket.on(
			'cancelAnalysis',
			makeServerListener('cancelAnalysis', async (analysisId, callback) => {
				const cancelHook = analysisCancelHooks.get(analysisId);
				if (cancelHook === undefined) {
					callback(false);
				} else {
					cancelHook.resolve();
					callback(true);
				}
			})
		);
		socket.on(
			'getStartFen',
			makeServerListener('getStartFen', async (_, callback) => {
				callback(startFen);
			})
		);
		socket.on('disconnect', () => {
			for (const gameId of listenerToGameId.get(socket.id) ?? []) {
				const remainingListeners = (listeners.get(gameId) ?? []).filter(
					(otherSocket) => otherSocket.id !== socket.id
				);
				if (remainingListeners.length === 0) {
					listeners.delete(gameId);
					games.delete(gameId);
					const cancelHook = gameCancelHooks.get(gameId);
					if (cancelHook !== undefined) {
						cancelHook.resolve();
						gameCancelHooks.delete(gameId);
					}
				} else {
					listeners.set(gameId, remainingListeners);
				}
			}

			for (const analysisId of analysesBySocket.get(socket.id) ?? []) {
				const cancelHook = analysisCancelHooks.get(analysisId);
				if (cancelHook !== undefined) {
					cancelHook.resolve();
				}
			}

			analysesBySocket.delete(socket.id);
		});
	});
})();
