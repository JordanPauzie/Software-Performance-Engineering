import { type MouseEvent } from 'react';
import PieceView from '@/features/board/components/piece-view';
import trash from '@/assets/trash.svg';
import pointer from '@/assets/pointer.svg';
import {
	type Direction,
	type Color,
	type Square,
	type Piece,
	squareEq,
	type OrdinalDirection,
} from '@/types/board';

type Properties = {
	readonly selectedSkittle: Square | undefined;
	readonly setSelectedSkittle: (skittle: Square | undefined) => void;
	readonly beginSkittleDrag: (event: MouseEvent, skittle: Piece) => void;
	readonly perspective: Color;
};

export default function PiecePalette({
	selectedSkittle,
	setSelectedSkittle,
	beginSkittleDrag,
	perspective,
}: Properties) {
	const pieceToPaletteIcon = (currentPiece: Piece, key: string) => (
		<svg
			key={key}
			className={`paletteIcon${selectedSkittle !== undefined && squareEq(selectedSkittle, currentPiece) ? ' selected' : ''}`}
			viewBox="0 0 1 1"
			xmlns="http://www.w3.org/2000/svg"
			onClick={() => {
				setSelectedSkittle(currentPiece);
			}}
			onMouseDown={(event) => {
				beginSkittleDrag(event, currentPiece);
			}}
		>
			<PieceView
				piece={currentPiece}
				boardPerspective={perspective}
				size={0.75}
				centerX={0.5}
				centerY={0.5}
			/>
		</svg>
	);
	return (
		<div className="piecePalette" draggable="false">
			<div className="controlIcons">
				<div
					className={`paletteIcon${selectedSkittle === undefined ? ' selected' : ''}`}
					onClick={() => {
						setSelectedSkittle(undefined);
					}}
				>
					<img className="palettePointer" src={pointer} />
				</div>
				<div
					className={`paletteIcon${selectedSkittle !== undefined && selectedSkittle.kind === 'empty' ? ' selected' : ''}`}
					onClick={() => {
						setSelectedSkittle({ kind: 'empty' });
					}}
				>
					<img className="paletteTrash" src={trash} />
				</div>
			</div>
			<div className="pieceIcons">
				{(['white', 'black'] as Color[]).map((color) => (
					<div key={color} className="colorGroup">
						<div className="pieceGroup">
							<div className="pieceRow">
								{(['north', 'east'] as Direction[]).map((direction) =>
									pieceToPaletteIcon(
										{
											kind: 'monarch',
											color,
											laserDirection: direction,
										},
										direction
									)
								)}
							</div>
							<div className="pieceRow">
								{(['west', 'south'] as Direction[]).map((direction) =>
									pieceToPaletteIcon(
										{
											kind: 'monarch',
											color,
											laserDirection: direction,
										},
										direction
									)
								)}
							</div>
						</div>
						<div className="pieceGroup">
							<div className="pieceRow">
								{(
									[
										['north', 'west'],
										['north', 'east'],
									] as OrdinalDirection[]
								).map((direction) =>
									pieceToPaletteIcon(
										{
											kind: 'pawn',
											color,
											mirrorDirection: direction,
										},
										`${direction[0]},${direction[1]}`
									)
								)}
							</div>
							<div className="pieceRow">
								{(
									[
										['south', 'west'],
										['south', 'east'],
									] as OrdinalDirection[]
								).map((direction) =>
									pieceToPaletteIcon(
										{
											kind: 'pawn',
											color,
											mirrorDirection: direction,
										},
										`${direction[0]},${direction[1]}`
									)
								)}
							</div>
						</div>
					</div>
				))}
			</div>
		</div>
	);
}
