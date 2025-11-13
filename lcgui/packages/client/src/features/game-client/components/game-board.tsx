import { useCallback, useEffect, useState, type MouseEvent } from 'react';
import { match } from 'ts-pattern';
import { getValidMoves, type ParsedMoveInfo } from '../api/client';
import { type Piece, type Board, type Color } from '@/types/board';
import { fileRankEq, type Move, moveEq, type Rotation, type FileRank } from '@/types/move';
import { useMouseDrag } from '@/hooks/use-mouse-drag';
import BoardView from '@/features/board/components/board-view';
import { rotateDirection, rotateOrdinalDirection } from '@/utils';

type Properties = {
	readonly board: Board;
	readonly keyframeBoards: Board[];
	readonly laserFrame: number;
	readonly lastMove?: ParsedMoveInfo;
	readonly selectMove?: (move: ParsedMoveInfo) => void;
	readonly playableColors: Color[];
	readonly defaultBoardPerspective: Color;
	readonly setPerspective?: (perspective: Color) => void;
};

const defaultBoardWidthPx = 500;

export default function GameBoard({
	board,
	keyframeBoards,
	laserFrame,
	lastMove,
	selectMove,
	playableColors,
	defaultBoardPerspective,
	setPerspective,
}: Properties) {
	const [selectedPiece, setSelectedPiece] = useState<
		| {
				piece: Piece;
				fileRank: FileRank;
				isDragged: boolean;
				hoveredMove: ParsedMoveInfo | undefined;
		  }
		| undefined
	>(undefined);
	const [validMoves, setValidMoves] = useState<ParsedMoveInfo[] | undefined>(undefined);
	const [draggedCoords, startDrag] = useMouseDrag(0, () => {
		setSelectedPiece((selectedPiece) =>
			match(selectedPiece)
				.with(undefined, () => undefined)
				.otherwise(({ piece, fileRank }) => ({
					piece,
					fileRank,
					isDragged: false,
					hoveredMove: undefined,
				}))
		);
	});

	useEffect(() => {
		const cancelValidMoves = getValidMoves(board, setValidMoves);
		return () => {
			cancelValidMoves();
		};
	}, [board]);

	const getValidMove = useCallback(
		(move: Move) => {
			if (validMoves !== undefined) {
				const selectedMove = validMoves.find((validMove) => moveEq(validMove.move, move));
				if (selectedMove !== undefined) {
					return selectedMove;
				}
			}

			return undefined;
		},
		[validMoves]
	);

	const mouseDownOnTile = useCallback(
		(event: MouseEvent, file: number, rank: number) => {
			if (event.button === 0) {
				if (selectedPiece?.hoveredMove === undefined) {
					const square = board.getSquare(file, rank);
					if (
						square.kind !== 'empty' &&
						square.color === board.playerToMove &&
						playableColors.includes(square.color)
					) {
						setSelectedPiece({
							piece: square,
							fileRank: [file, rank],
							isDragged: true,
							hoveredMove: undefined,
						});
						startDrag(event);
					} else {
						setSelectedPiece(undefined);
					}
				} else if (
					!selectedPiece.isDragged &&
					selectedPiece.hoveredMove !== undefined &&
					selectMove !== undefined
				) {
					selectMove(selectedPiece.hoveredMove);
					setSelectedPiece(undefined);
				}
			}
		},
		[board, selectedPiece, startDrag, selectMove, playableColors]
	);
	const mouseUpOnTile = useCallback(
		(event: MouseEvent) => {
			if (
				event.button === 0 &&
				selectedPiece !== undefined &&
				selectedPiece.isDragged &&
				selectedPiece.hoveredMove !== undefined &&
				selectMove !== undefined
			) {
				selectMove(selectedPiece.hoveredMove);
				setSelectedPiece(undefined);
			}
		},
		[selectedPiece, selectMove]
	);
	const mouseMoveOnTile = useCallback(
		(_: MouseEvent, file: number, rank: number) => {
			setSelectedPiece((selectedPiece) => {
				if (selectedPiece !== undefined) {
					if (fileRankEq([file, rank], selectedPiece.fileRank)) return selectedPiece;
					const hoveredMove = getValidMove({
						kind: 'push',
						from: selectedPiece.fileRank,
						to: [file, rank],
					});
					return { ...selectedPiece, hoveredMove };
				}

				return undefined;
			});
		},
		[getValidMove, setSelectedPiece]
	);

	const hoverOnRotation = useCallback(
		(rotation?: Rotation) => {
			setSelectedPiece((selectedPiece) => {
				if (selectedPiece !== undefined) {
					const hoveredMove =
						rotation === undefined
							? undefined
							: getValidMove({
									kind: 'rotate',
									from: selectedPiece.fileRank,
									rotation,
								});
					return { ...selectedPiece, hoveredMove };
				}

				return undefined;
			});
		},
		[getValidMove, setSelectedPiece]
	);

	const dottedSquares =
		selectedPiece !== undefined && validMoves !== undefined
			? validMoves.flatMap((validMove) =>
					fileRankEq(validMove.move.from, selectedPiece.fileRank) && validMove.move.kind === 'push'
						? [validMove.move.to]
						: []
				)
			: [];

	return (
		<BoardView
			board={board}
			keyframeBoards={keyframeBoards}
			laserFrame={laserFrame}
			defaultWidth={defaultBoardWidthPx}
			defaultBoardPerspective={defaultBoardPerspective}
			mouseDownOnTile={mouseDownOnTile}
			mouseUpOnTile={mouseUpOnTile}
			mouseMoveOnTile={mouseMoveOnTile}
			floatingPieces={
				selectedPiece !== undefined && draggedCoords !== undefined && selectedPiece.isDragged
					? [{ piece: selectedPiece.piece, absoluteCoords: draggedCoords, id: 1 }]
					: []
			}
			ghostPieces={(() => {
				if (selectedPiece !== undefined) {
					if (selectedPiece.isDragged) {
						return [{ piece: selectedPiece.piece, position: selectedPiece.fileRank }];
					}

					if (
						selectedPiece.hoveredMove !== undefined &&
						selectedPiece.hoveredMove.move.kind === 'rotate'
					) {
						const rotation = selectedPiece.hoveredMove.move.rotation;

						return [
							{
								piece: match(selectedPiece.piece)
									.with({ kind: 'monarch' }, (monarch) => ({
										...monarch,
										laserDirection: rotateDirection(monarch.laserDirection, rotation),
									}))
									.with({ kind: 'pawn' }, (pawn) => ({
										...pawn,
										mirrorDirection: rotateOrdinalDirection(pawn.mirrorDirection, rotation),
									}))
									.exhaustive(),
								position: selectedPiece.fileRank,
							},
						];
					}
				}

				return [];
			})()}
			maskedPieces={
				selectedPiece !== undefined &&
				(selectedPiece.isDragged ||
					(selectedPiece.hoveredMove !== undefined &&
						selectedPiece.hoveredMove.move.kind === 'rotate'))
					? [selectedPiece.fileRank]
					: []
			}
			highlightedSquares={[
				...dottedSquares.map((square) => ({ square, highlighting: 'dotted' as const })),
				...(lastMove === undefined
					? []
					: lastMove.move.kind === 'push'
						? [
								{ square: lastMove.move.from, highlighting: 'lastMove' as const },
								{ square: lastMove.move.to, highlighting: 'lastMove' as const },
							]
						: [{ square: lastMove.move.from, highlighting: 'lastMove' as const }]),
			]}
			rotationTile={
				selectedPiece !== undefined && validMoves !== undefined && !selectedPiece.isDragged
					? {
							position: selectedPiece.fileRank,
							validRotations: validMoves.flatMap((validMove) =>
								fileRankEq(validMove.move.from, selectedPiece.fileRank) &&
								validMove.move.kind === 'rotate'
									? [validMove.move.rotation]
									: []
							),
						}
					: undefined
			}
			hoverOnRotation={hoverOnRotation}
			laser={
				selectedPiece?.hoveredMove === undefined
					? lastMove === undefined
						? undefined
						: {
								...lastMove.laser,
								color: board.playerToMove === 'white' ? 'black' : 'white',
								isTentative: false,
							}
					: { ...selectedPiece.hoveredMove.laser, color: board.playerToMove, isTentative: true }
			}
			setPerspective={setPerspective}
		/>
	);
}
