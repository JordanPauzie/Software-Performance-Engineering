import { match } from 'ts-pattern';
import { type Color, type Piece } from '@/types/board';
import blackKing from '@/assets/king-black.svg';
import whiteKing from '@/assets/king-white.svg';
import blackPawn from '@/assets/pawn-black.svg';
import whitePawn from '@/assets/pawn-white.svg';

type Properties = {
	readonly piece: Piece;
	readonly boardPerspective: Color;
	readonly size: number;
	readonly centerX: number;
	readonly centerY: number;
	readonly opacity?: number;
};

export default function PieceView({
	piece,
	boardPerspective,
	size,
	centerX,
	centerY,
	opacity,
}: Properties) {
	const x = centerX - size / 2;
	const y = centerY - size / 2;
	let rotation;
	let href;
	if (piece.kind === 'pawn') {
		if (piece.mirrorDirection[0] === 'north') {
			rotation = piece.mirrorDirection[1] === 'east' ? 90 : 0;
		} else {
			rotation = piece.mirrorDirection[1] === 'east' ? 180 : 270;
		}

		href = piece.color === 'black' ? blackPawn : whitePawn;
	} else {
		rotation = match(piece.laserDirection)
			.with('north', () => 0)
			.with('east', () => 90)
			.with('south', () => 180)
			.with('west', () => 270)
			.exhaustive();

		href = piece.color === 'black' ? blackKing : whiteKing;
	}

	rotation += match(boardPerspective)
		.with('black', () => 180)
		.with('white', () => 0)
		.exhaustive();
	rotation %= 360;

	return (
		<image
			className="boardPiece"
			href={href}
			width={size}
			height={size}
			x={x}
			y={y}
			transform={`rotate(${rotation})`}
			opacity={opacity}
		/>
	);
}
