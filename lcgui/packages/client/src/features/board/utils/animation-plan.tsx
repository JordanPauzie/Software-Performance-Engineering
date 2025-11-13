import assert from 'node:assert';
import { match } from 'ts-pattern';
import { type Piece, squareEq, type Board, type Color, colors, pieceKinds } from '@/types/board';
import { type Rotation, type FileRank } from '@/types/move';
import { computeDirectionRotation, computeOrdinalDirectionRotation, forRange } from '@/utils';

export type Animation =
	| { kind: 'pushRotate'; piece: Piece; from: FileRank; to: FileRank; rotation: Rotation }
	| { kind: 'fadeOut'; piece: Piece; from: FileRank }
	| { kind: 'fadeIn'; piece: Piece; from: FileRank };

type MismatchedPiece = { piece: Piece; fileRank: FileRank };

export function getAnimations(currentFrame: Board, nextFrame: Board): Animation[] {
	assert(currentFrame.files === nextFrame.files && currentFrame.ranks === nextFrame.ranks);
	const animations: Animation[] = [];
	const mismatchedCurrentPieces: Record<Color, Record<Piece['kind'], MismatchedPiece[]>> = {
		white: { pawn: [], monarch: [] },
		black: { pawn: [], monarch: [] },
	};
	const mismatchedNextPieces: Record<Color, Record<Piece['kind'], MismatchedPiece[]>> = {
		white: { pawn: [], monarch: [] },
		black: { pawn: [], monarch: [] },
	};
	const pairMismatches = (currentMismatch: MismatchedPiece, nextMismatch: MismatchedPiece) => {
		assert(currentMismatch.piece.color === nextMismatch.piece.color);
		animations.push({
			kind: 'pushRotate',
			from: currentMismatch.fileRank,
			to: nextMismatch.fileRank,
			piece: currentMismatch.piece,
			rotation: match([currentMismatch.piece, nextMismatch.piece])
				.with(
					[{ kind: 'pawn' }, { kind: 'pawn' }],
					([{ mirrorDirection: currentDirection }, { mirrorDirection: nextDirection }]) =>
						computeOrdinalDirectionRotation(currentDirection, nextDirection)
				)
				.with(
					[{ kind: 'monarch' }, { kind: 'monarch' }],
					([{ laserDirection: currentDirection }, { laserDirection: nextDirection }]) =>
						computeDirectionRotation(currentDirection, nextDirection)
				)
				.otherwise(() => {
					assert(false);
				}),
		});
	};

	forRange(currentFrame.files, (file) =>
		forRange(currentFrame.ranks, (rank) => {
			const currentSquare = currentFrame.getSquare(file, rank);
			const nextSquare = nextFrame.getSquare(file, rank);
			if (squareEq(currentSquare, nextSquare)) return;
			if (
				currentSquare.kind === nextSquare.kind &&
				currentSquare.kind !== 'empty' &&
				nextSquare.kind !== 'empty' &&
				currentSquare.color === nextSquare.color
			) {
				pairMismatches(
					{ piece: currentSquare, fileRank: [file, rank] },
					{ piece: nextSquare, fileRank: [file, rank] }
				);
			} else {
				if (currentSquare.kind !== 'empty') {
					const mismatch: MismatchedPiece = { piece: currentSquare, fileRank: [file, rank] };
					const otherMismatch = mismatchedNextPieces[currentSquare.color][currentSquare.kind].pop();
					if (otherMismatch === undefined) {
						mismatchedCurrentPieces[currentSquare.color][currentSquare.kind].push(mismatch);
					} else {
						pairMismatches(mismatch, otherMismatch);
					}
				}

				if (nextSquare.kind !== 'empty') {
					const mismatch: MismatchedPiece = { piece: nextSquare, fileRank: [file, rank] };
					const otherMismatch = mismatchedCurrentPieces[nextSquare.color][nextSquare.kind].pop();
					if (otherMismatch === undefined) {
						mismatchedNextPieces[nextSquare.color][nextSquare.kind].push(mismatch);
					} else {
						pairMismatches(otherMismatch, mismatch);
					}
				}
			}
		})
	);
	for (const color of colors) {
		for (const pieceKind of pieceKinds) {
			for (const { fileRank } of mismatchedCurrentPieces[color][pieceKind]) {
				const piece = currentFrame.getSquare(...fileRank);
				assert(piece.kind !== 'empty');
				animations.push({
					kind: 'fadeOut',
					piece,
					from: fileRank,
				});
			}

			for (const { fileRank } of mismatchedNextPieces[color][pieceKind]) {
				const piece = nextFrame.getSquare(...fileRank);
				assert(piece.kind !== 'empty');
				animations.push({
					kind: 'fadeIn',
					piece,
					from: fileRank,
				});
			}
		}
	}

	return animations;
}
