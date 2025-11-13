import { strict as assert } from 'node:assert';
import { match } from 'ts-pattern';
import { getFileString, parseFileString } from '@/utils';

export type FileRank = [number, number];
export const rotations = ['nullMove', 'R', 'U', 'L'];
export type Rotation = (typeof rotations)[number];
export type Move =
	| { kind: 'push'; from: FileRank; to: FileRank }
	| { kind: 'rotate'; from: FileRank; rotation: Rotation };

export function fileRankEq(a: FileRank, b: FileRank): boolean {
	return a[0] === b[0] && a[1] === b[1];
}

export function moveEq(a: Move, b: Move): boolean {
	return match([a, b])
		.with(
			[{ kind: 'push' }, { kind: 'push' }],
			([{ from: aFrom, to: aTo }, { from: bFrom, to: bTo }]) =>
				fileRankEq(aFrom, bFrom) && fileRankEq(aTo, bTo)
		)
		.with(
			[{ kind: 'rotate' }, { kind: 'rotate' }],
			([{ from: aFrom, rotation: aRotation }, { from: bFrom, rotation: bRotation }]) =>
				fileRankEq(aFrom, bFrom) && aRotation === bRotation
		)
		.otherwise(() => false);
}

export function parseFileRankString(fileRankString: string): FileRank {
	assert(fileRankString.length === 2);
	return [parseFileString(fileRankString[0]!), Number.parseInt(fileRankString[1]!, 10)];
}

export function parseMoveString(moveString: string): Move {
	assert(moveString.length === 3 || moveString.length === 4);
	const from: FileRank = parseFileRankString(moveString.slice(0, 2));
	if (moveString.length === 3) {
		const rotation = rotations.find((rotation) => rotation === moveString[2]!);
		assert(rotation !== undefined);
		return { kind: 'rotate', from, rotation };
	}

	const to: FileRank = parseFileRankString(moveString.slice(2, 4));
	if (fileRankEq(from, to)) {
		return { kind: 'rotate', from, rotation: 'nullMove' };
	}

	return { kind: 'push', from, to };
}

export function getMoveString(move: Move): string {
	return match(move)
		.with(
			{ kind: 'push' },
			({ from: [fromFile, fromRank], to: [toFile, toRank] }) =>
				`${getFileString(fromFile)}${fromRank}${getFileString(toFile)}${toRank}`
		)
		.with({ kind: 'rotate' }, ({ from: [fromFile, fromRank], rotation }) =>
			rotation === 'nullMove'
				? `${getFileString(fromFile)}${fromRank}${getFileString(fromFile)}${fromRank}`
				: `${getFileString(fromFile)}${fromRank}${rotation}`
		)
		.exhaustive();
}
