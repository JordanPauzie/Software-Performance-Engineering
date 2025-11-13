import { strict as assert } from 'node:assert';
import { match } from 'ts-pattern';
import { forRange } from '@/utils';

export const fileDirections = ['north', 'south'] as const;
export const rankDirections = ['east', 'west'] as const;
export const directions = [...fileDirections, ...rankDirections] as const;
export type FileDirection = (typeof fileDirections)[number];
export type RankDirection = (typeof rankDirections)[number];
export type Direction = (typeof directions)[number];

export function opposite(d: Direction): Direction {
	return match<Direction, Direction>(d)
		.with('north', () => 'south')
		.with('east', () => 'west')
		.with('south', () => 'north')
		.with('west', () => 'east')
		.exhaustive();
}

export function takeStep(direction: Direction, file: number, rank: number): [number, number] {
	return match<Direction, [number, number]>(direction)
		.with('north', () => [file, rank + 1])
		.with('east', () => [file + 1, rank])
		.with('south', () => [file, rank - 1])
		.with('west', () => [file - 1, rank])
		.exhaustive();
}

export function isFileDirection(d: Direction): d is FileDirection {
	return d === 'north' || d === 'south';
}

export function abbreviate(d: Direction): string {
	return match(d)
		.with('north', () => 'N')
		.with('east', () => 'E')
		.with('south', () => 'S')
		.with('west', () => 'W')
		.exhaustive();
}

export function unabbreviate(directionString: string): Direction | undefined {
	return match<string, Direction | undefined>(directionString)
		.with('N', () => 'north')
		.with('E', () => 'east')
		.with('S', () => 'south')
		.with('W', () => 'west')
		.otherwise(() => undefined);
}

export type OrdinalDirection = [FileDirection, RankDirection];

export const colors = ['white', 'black'] as const;
export type Color = (typeof colors)[number];

export const pieceKinds = ['pawn', 'monarch'] as const;
export type Piece =
	| { kind: 'pawn'; mirrorDirection: OrdinalDirection; color: Color }
	| { kind: 'monarch'; laserDirection: Direction; color: Color };

export type Square = Piece | { kind: 'empty' };

export function squareEq(a: Square, b: Square): boolean {
	return match([a, b])
		.with(
			[{ kind: 'pawn' }, { kind: 'pawn' }],
			([a, b]) =>
				a.color === b.color &&
				a.mirrorDirection[0] === b.mirrorDirection[0] &&
				a.mirrorDirection[1] === b.mirrorDirection[1]
		)
		.with(
			[{ kind: 'monarch' }, { kind: 'monarch' }],
			([a, b]) => a.color === b.color && a.laserDirection === b.laserDirection
		)
		.with([{ kind: 'empty' }, { kind: 'empty' }], () => true)
		.otherwise(() => false);
}

export class Board {
	private readonly squares: Square[][];
	public constructor(
		public readonly files: number,
		public readonly ranks: number,
		public readonly playerToMove: Color,
		squareGetter?: (file: number, rank: number) => Square
	) {
		if (squareGetter === undefined) {
			this.squares = forRange(ranks, () =>
				forRange(files, () => ({
					kind: 'empty',
				}))
			);
		} else {
			this.squares = forRange(ranks, (rank) => forRange(files, (file) => squareGetter(file, rank)));
		}
	}

	public withinBorders(file: number, rank: number): boolean {
		return file >= 0 && file <= this.files && rank >= 0 && rank < this.ranks;
	}

	public getSquare(file: number, rank: number): Square {
		assert(this.withinBorders(file, rank));
		return this.squares[rank]![file]!;
	}

	public setSquare(file: number, rank: number, square: Square): Board {
		assert(this.withinBorders(file, rank));
		return new Board(this.files, this.ranks, this.playerToMove, (f, r) =>
			f === file && r === rank ? square : this.getSquare(f, r)
		);
	}

	public toggleMove(): Board {
		return new Board(
			this.files,
			this.ranks,
			this.playerToMove === 'white' ? 'black' : 'white',
			(f, r) => this.getSquare(f, r)
		);
	}

	public toFen(): string {
		const fenRows: string[] = [];
		for (const row of this.squares) {
			const fenPieces: string[] = [];
			let counter = 0;
			for (const square of row) {
				if (square.kind === 'empty') {
					counter++;
				} else {
					if (counter !== 0) {
						fenPieces.push(`${counter}`);
					}

					counter = 0;
					const directionString = match(square)
						.with(
							{ kind: 'pawn' },
							({ mirrorDirection }) =>
								`${abbreviate(mirrorDirection[0])}${abbreviate(mirrorDirection[1])}`
						)
						.otherwise(
							({ laserDirection }) => `${abbreviate(laserDirection)}${abbreviate(laserDirection)}`
						);

					fenPieces.push(
						square.color === 'white' ? directionString : directionString.toLowerCase()
					);
				}
			}

			if (counter !== 0) {
				fenPieces.push(`${counter}`);
			}

			fenRows.push(fenPieces.join(''));
		}

		fenRows.reverse();
		return `${fenRows.join('/')} ${this.playerToMove === 'white' ? 'W' : 'B'}`;
	}
}

export function fenToBoard(fen: string): Board | undefined {
	let files;
	const rows: Array<Map<number, Piece>> = [];
	let playerToMove: Color;
	if (fen.endsWith(' W')) {
		playerToMove = 'white';
	} else if (fen.endsWith(' B')) {
		playerToMove = 'black';
	} else {
		return undefined;
	}

	fen = fen.slice(0, Math.max(0, fen.length - 2));
	for (const row of fen.split('/')) {
		const pieces = new Map<number, Piece>();
		let rowIndex = 0;
		const pattern = /(?<whitePiece>[NSEW]{2})|(?<blackPiece>[nsew]{2})|(?<num>0|[1-9]\d*)/g;
		while (pattern.lastIndex < row.length) {
			const nextIndex = pattern.lastIndex;
			const regexMatch = pattern.exec(row);
			if (regexMatch === null || regexMatch.index !== nextIndex) {
				return undefined;
			}

			const { whitePiece, blackPiece, num } = regexMatch.groups!;
			let color: Color;
			let pieceString;
			if (whitePiece) {
				color = 'white';
				pieceString = whitePiece;
			} else if (blackPiece) {
				color = 'black';
				pieceString = blackPiece;
			} else {
				assert(num !== undefined);
				rowIndex += Number.parseInt(num, 10);
				continue;
			}

			const [pieceDirection1, pieceDirection2] = Array.from(
				pieceString.toUpperCase(),
				unabbreviate
			);
			let piece: Piece;
			if (pieceDirection1 === undefined || pieceDirection2 === undefined) {
				return undefined;
			}

			if (pieceDirection1 === pieceDirection2) {
				piece = { kind: 'monarch', laserDirection: pieceDirection1, color };
			} else {
				const fileDirection = fileDirections.find((a) => a === pieceDirection1);
				const rankDirection = rankDirections.find((a) => a === pieceDirection2);
				if (fileDirection !== undefined && rankDirection !== undefined) {
					piece = {
						kind: 'pawn',
						mirrorDirection: [fileDirection, rankDirection],
						color,
					};
				} else {
					return undefined;
				}
			}

			pieces.set(rowIndex++, piece);
		}

		rows.push(pieces);
		if (files === undefined) {
			files = rowIndex;
		} else if (files !== rowIndex) {
			return undefined;
		}
	}

	return new Board(
		files ?? 0,
		rows.length,
		playerToMove,
		(file, rank) => rows[rows.length - 1 - rank]!.get(file) ?? { kind: 'empty' }
	);
}
