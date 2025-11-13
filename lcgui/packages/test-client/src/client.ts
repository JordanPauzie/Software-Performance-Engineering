/* eslint-disable unicorn/prefer-top-level-await */
/* eslint-disable no-await-in-loop */
import readline from 'node:readline';
import { stdin, stdout } from 'node:process';
import { io, type Socket } from 'socket.io-client';
import {
	type ClientToServerEvents,
	type ClientToServerHandler,
	clientToServerSchemas,
	makeClientCallback,
	makeClientListener,
	type ServerToClientEvents,
} from '@lcgui/shared';

(async () => {
	const socket: Socket<ServerToClientEvents, ClientToServerEvents> = io('http://localhost:3000');
	const rl = readline.createInterface({ input: stdin });
	async function readInputLine(): Promise<string> {
		const iterator = rl[Symbol.asyncIterator]();
		const next = await iterator.next();
		if (!next.done) {
			return next.value;
		}

		throw new Error('ended');
	}

	socket.on('connect', () => {
		console.log('Connected!');
		for (const message of ['analysisReported', 'moveMade', 'gameEnded'] as const) {
			socket.on(
				message,
				makeClientListener(message, (argument) => {
					console.log(`${message}: ${JSON.stringify(argument)}`);
				})
			);
		}
	});

	async function getEmitArguments<T extends keyof ClientToServerEvents>(
		clientMessage: T
	): Promise<[T, ...Parameters<ClientToServerHandler<T>>]> {
		stdout.write('\t(args) ');
		const messageArguments = await readInputLine();
		return [
			clientMessage,
			clientToServerSchemas[clientMessage].arguments.parse(JSON.parse(messageArguments)),
			makeClientCallback(clientMessage, (result) => {
				console.log(`result: ${JSON.stringify(result)}`);
			}),
		];
	}

	while (true) {
		stdout.write('> ');
		const clientMessage = await readInputLine();
		switch (clientMessage) {
			case 'startGame': {
				socket.emit(...(await getEmitArguments('startGame')));
				break;
			}

			case 'move': {
				socket.emit(...(await getEmitArguments('move')));
				break;
			}

			case 'validMoves': {
				socket.emit(...(await getEmitArguments('validMoves')));
				break;
			}

			case 'listenGame': {
				socket.emit(...(await getEmitArguments('listenGame')));
				break;
			}

			case 'analyze': {
				socket.emit(...(await getEmitArguments('analyze')));
				break;
			}

			case 'cancelAnalysis': {
				socket.emit(...(await getEmitArguments('cancelAnalysis')));
				break;
			}

			case 'getStartFen': {
				socket.emit(...(await getEmitArguments('getStartFen')));
				break;
			}

			default: {
				console.log('bad message type');
				break;
			}
		}
	}
})();
