/* eslint-disable no-await-in-loop */
import { type ChildProcessWithoutNullStreams, spawn } from 'node:child_process';
import readline from 'node:readline';
import { moveInfo, type MoveInfo } from '@lcgui/shared';
import { Deferred } from './deferred';

const binaryPath = './resources/referee';

export class RefereeBinary {
	private lc: ChildProcessWithoutNullStreams;
	private lcOutReader: readline.Interface;
	private lcKilled: Deferred<undefined>;
	public constructor() {
		this.lc = spawn(binaryPath);
		this.lcOutReader = readline.createInterface(this.lc.stdout);
		this.lcKilled = new Deferred<undefined>();
	}

	public writeLine(line: string): void {
		this.lc.stdin.write(line);
	}

	public async readOutputLine(): Promise<string> {
		const iterator = this.lcOutReader[Symbol.asyncIterator]();
		const next = await iterator.next();
		if (!next.done) {
			return next.value;
		}

		throw new Error('referee has died');
	}

	public async *readMoveInfo(fen: string): AsyncGenerator<MoveInfo, void, void> {
		if (fen.includes('\n') || fen.includes('\r')) return;
		this.lc.stdin.write(`validmoves ${fen}\n`);
		const buffer = [];
		for await (const line of this.readLines()) {
			if (line.startsWith('start')) {
				continue;
			} else if (line.startsWith('end')) {
				break;
			} else {
				const [move, laserStart, laserSteps, keyframe, endState, status] = line.trim().split(',');
				const moveInfoParse = moveInfo.safeParse({
					move,
					endState,
					keyframe,
					status,
					laser: { start: laserStart, steps: laserSteps?.split('') },
				});
				if (moveInfoParse.success) {
					buffer.push(moveInfoParse.data);
				}
			}
		}

		for (const moveInfo of buffer) {
			yield moveInfo;
		}
	}

	public async getBestMove(fen: string, searchCommand: string): Promise<string | undefined> {
		if (fen.includes('\n') || fen.includes('\r')) return undefined;
		this.writeLine(`fenvalid ${fen}\n`);
		const validOut = await this.readOutputLine();
		if (validOut.trim() === 'valid') {
			this.writeLine(`position fen ${fen}\n${searchCommand}`);
			for await (const line of this.readLines()) {
				if (line.startsWith('bestmove')) {
					return line.trim().split(' ')[1]!;
				}
			}
		}

		return undefined;
	}

	public restartBinary(): void {
		const killSucceeded = this.lc.kill('SIGKILL');
		if (killSucceeded) {
			this.lcKilled.resolve(undefined);
			this.lcKilled = new Deferred<undefined>();
			this.lc = spawn(binaryPath);
			this.lcOutReader = readline.createInterface(this.lc.stdout);
		} else {
			throw new Error('could not kill referee');
		}
	}

	private async *readLines(): AsyncGenerator<string, void, void> {
		const killed = this.lcKilled.promise;
		while (true) {
			const output = await Promise.any([killed, this.readOutputLine()]);
			if (output === undefined) {
				break;
			} else {
				yield output;
			}
		}
	}
}
