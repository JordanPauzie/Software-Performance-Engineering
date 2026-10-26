/* eslint-disable max-params */
import assert from 'node:assert';

export class GameClock {
	private runningTimeLimit: number;
	private pausedTimeLeft: number;
	private timeout: Promise<void>;
	private timeoutHandle: NodeJS.Timeout;
	private timeoutTriggered: boolean;
	public constructor(
		whiteLimit: number,
		blackLimit: number,
		private readonly whiteIncrement: number,
		private readonly blackIncrement: number,
		private runningColor: 'white' | 'black',
		private readonly forfeit: (color: 'white' | 'black') => void
	) {
		this.runningTimeLimit =
			Date.now() + (runningColor === 'white' ? whiteLimit : blackLimit) * 1000;
		this.pausedTimeLeft = runningColor === 'white' ? blackLimit : whiteLimit;
		this.timeoutTriggered = false;
		let timeoutHandle;
		this.timeout = new Promise((resolve) => {
			timeoutHandle = setTimeout(() => {
				this.timeoutTriggered = true;
				this.forfeit(this.runningColor);
				resolve();
			}, whiteLimit * 1000);
		});
		assert(timeoutHandle !== undefined);
		this.timeoutHandle = timeoutHandle;
	}

	public async waitTimeout(): Promise<void> {
		await this.timeout;
	}

	public getTimeLeft(): [number, number] {
		const runningTimeLeft = Math.max(0, this.runningTimeLimit - Date.now()) / 1000;
		return this.runningColor === 'white'
			? [runningTimeLeft, this.pausedTimeLeft]
			: [this.pausedTimeLeft, runningTimeLeft];
	}

	public toggleSide(): boolean {
		const timestamp = Date.now();
		const runningTimeLeft = this.runningTimeLimit - timestamp;
		if (runningTimeLeft < 0 || this.timeoutTriggered) {
			return false;
		}

		clearTimeout(this.timeoutHandle);

		const newPaused =
			runningTimeLeft / 1000 +
			(this.runningColor === 'white' ? this.whiteIncrement : this.blackIncrement);
		const newRunning = this.pausedTimeLeft * 1000 + timestamp;

		this.runningColor = this.runningColor === 'white' ? 'black' : 'white';
		let timeoutHandle;
		this.timeout = new Promise((resolve) => {
			timeoutHandle = setTimeout(() => {
				this.timeoutTriggered = true;
				this.forfeit(this.runningColor);
				resolve();
			}, this.pausedTimeLeft * 1000);
		});
		assert(timeoutHandle !== undefined);
		this.timeoutHandle = timeoutHandle;

		this.runningTimeLimit = newRunning;
		this.pausedTimeLeft = newPaused;
		return true;
	}

	public getRunningTime(): number {
		return Math.max(0, this.runningTimeLimit - Date.now());
	}

	public getRunningIncrement(): number {
		return (this.runningColor === 'white' ? this.whiteIncrement : this.blackIncrement) * 1000;
	}
}
