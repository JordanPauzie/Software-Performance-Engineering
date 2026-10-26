import { useState, useCallback, type MouseEvent } from 'react';
import PiecePalette from '../components/piece-palette';
import { Board, type Square, type Piece, type Color } from '@/types/board';
import BoardView from '@/features/board/components/board-view';
import { useMouseDrag } from '@/hooks/use-mouse-drag';
import FenInput from '@/features/board/components/fen-input';

const defaultBoardSize = 8;
const defaultBoardWidthPx = 500;

export default function Skittles() {
	const [board, setBoard] = useState(new Board(defaultBoardSize, defaultBoardSize, 'white'));

	const [draggedCoords, startDrag] = useMouseDrag(0, () => {
		setDraggedSkittle(undefined);
	});
	const [selectedSkittle, setSelectedSkittle] = useState<Square | undefined>(undefined);
	const [draggedSkittle, setDraggedSkittle] = useState<Piece | undefined>(undefined);
	const [perspective, setPerspective] = useState<Color>('white');
	const handlePieceGrab = useCallback(
		(event: MouseEvent, file: number, rank: number) => {
			if (event.button === 0) {
				if (selectedSkittle === undefined) {
					const square = board.getSquare(file, rank);
					// Piece must be moveable in the current turn to be draggable or the board must be a skittles board
					if (square.kind !== 'empty') {
						setBoard(board.setSquare(file, rank, { kind: 'empty' }));
						setDraggedSkittle(square);
					}
				} else {
					setBoard(board.setSquare(file, rank, selectedSkittle));
				}

				startDrag(event);
			}
		},
		[board, selectedSkittle, startDrag]
	);
	const handlePieceDrop = useCallback(
		(event: MouseEvent, file: number, rank: number) => {
			if (event.button === 0 && draggedSkittle !== undefined) {
				setBoard(board.setSquare(file, rank, draggedSkittle));
				setDraggedSkittle(undefined);
			}
		},
		[draggedSkittle, board]
	);
	const placeSelectedSkittle = useCallback(
		(_: MouseEvent, file: number, rank: number) => {
			if (
				draggedCoords !== undefined &&
				selectedSkittle !== undefined &&
				draggedSkittle === undefined
			) {
				setBoard(board.setSquare(file, rank, selectedSkittle));
			}
		},
		[board, selectedSkittle, draggedSkittle, draggedCoords]
	);
	const beginSkittleDrag = useCallback(
		(event: MouseEvent, skittle: Piece) => {
			startDrag(event);
			setDraggedSkittle(skittle);
		},
		[startDrag]
	);
	return (
		<div className="skittles">
			<div className="skittlesInput">
				<BoardView
					defaultWidth={defaultBoardWidthPx}
					setPerspective={setPerspective}
					board={board}
					keyframeBoards={[]}
					laserFrame={0}
					defaultBoardPerspective="white"
					mouseDownOnTile={handlePieceGrab}
					mouseUpOnTile={handlePieceDrop}
					mouseMoveOnTile={placeSelectedSkittle}
					floatingPieces={
						draggedSkittle !== undefined && draggedCoords !== undefined
							? [{ piece: draggedSkittle, absoluteCoords: draggedCoords, id: 1 }]
							: []
					}
				/>
				<PiecePalette
					selectedSkittle={selectedSkittle}
					setSelectedSkittle={setSelectedSkittle}
					beginSkittleDrag={beginSkittleDrag}
					perspective={perspective}
				/>
			</div>
			<FenInput board={board} setBoard={setBoard} />
		</div>
	);
}
