import {
	type ClientToServerArgument,
	type GameInfo,
	GameClock,
	type MoveInfo,
	type GameStatus,
} from '@lcgui/shared';

export type GameControl =
	| {
			kind: 'time';
			clock: GameClock;
	  }
	| {
			kind: 'depth';
			depth: number;
	  };

export class GameState {
	public readonly userPlaysWhite: boolean;
	private fen: string;
	private readonly moveHistory: MoveInfo[];
	private readonly controls: GameControl;
	private status: GameStatus;
	public constructor(
		startFen: string,
		public readonly gameId: number,
		{ control, userPlaysWhite }: ClientToServerArgument<'startGame'>,
		forfeit: (color: 'white' | 'black') => void
	) {
		this.fen = startFen;
		this.moveHistory = [];
		this.controls =
			control.kind === 'time'
				? {
						kind: 'time',
						clock: new GameClock(
							control.whiteLimit,
							control.blackLimit,
							control.whiteIncrement,
							control.blackIncrement,
							'white',
							forfeit
						),
					}
				: { kind: 'depth', depth: control.depth };
		this.userPlaysWhite = userPlaysWhite;
		this.status = 'ongoing';
	}

	public addMoveToState(move: MoveInfo, fen: string): boolean {
		if (this.controls.kind === 'time' && !this.controls.clock.toggleSide()) {
			return false;
		}

		this.fen = fen;
		this.moveHistory.push(move);
		this.status = move.status;
		return true;
	}

	public getStatus(): GameStatus {
		return this.status;
	}

	public getFen(): string {
		return this.fen;
	}

	public getGameInfo(): GameInfo {
		let whiteTime;
		let blackTime;
		if (this.controls.kind === 'time') {
			[whiteTime, blackTime] = this.controls.clock.getTimeLeft();
		}

		return {
			gameId: this.gameId,
			fen: this.fen,
			whiteTime,
			blackTime,
			moveHistory: this.moveHistory,
			status: this.status,
		};
	}

	public isCpuTurn(): boolean {
		return this.userPlaysWhite !== (this.moveHistory.length % 2 === 0);
	}

	public getCpuSearchCommand(): string {
		return this.controls.kind === 'depth'
			? `go depth ${this.controls.depth}\n`
			: `go time ${this.controls.clock.getRunningTime()} inc ${this.controls.clock.getRunningIncrement()}\n`;
	}
}
