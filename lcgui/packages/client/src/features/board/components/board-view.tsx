import { useRef, useState, useCallback, type MouseEvent, useMemo } from 'react';
import { Transition, TransitionGroup } from 'react-transition-group';
import { match } from 'ts-pattern';
import { getAnimations } from '../utils/animation-plan';
import PieceView from './piece-view';
import BoardBackground from './board-background';
import Annotations from './annotation';
import LaserView from './laser-view';
import RotationPicker from './rotation-picker';
import { type Color, type Board, type Piece, type Direction } from '@/types/board';
import { forRange, getFileString, rotateDirection, rotateOrdinalDirection } from '@/utils';
import { type FileRank, type Rotation } from '@/types/move';
import { useMouseDrag } from '@/hooks/use-mouse-drag';

type Properties = {
	readonly board: Board;
	readonly keyframeBoards: Board[];
	readonly laserFrame: number;
	readonly defaultWidth: number;
	readonly defaultBoardPerspective: Color;
	readonly mouseDownOnTile?: (event: MouseEvent, file: number, rank: number) => void;
	readonly mouseUpOnTile?: (event: MouseEvent, file: number, rank: number) => void;
	readonly mouseMoveOnTile?: (event: MouseEvent, file: number, rank: number) => void;
	readonly floatingPieces?: Array<{ piece: Piece; absoluteCoords: [number, number]; id: number }>;
	readonly ghostPieces?: Array<{ piece: Piece; position: FileRank }>;
	readonly maskedPieces?: FileRank[];
	readonly highlightedSquares?: Array<{
		square: FileRank;
		highlighting: 'lastMove' | 'current' | 'dotted';
	}>;
	readonly rotationTile?: { position: FileRank; validRotations: Rotation[] };
	readonly hoverOnRotation?: (rotation?: Rotation) => void;
	readonly laser?: {
		color: Color;
		steps: Direction[];
		isTentative: boolean;
		start: FileRank;
	};
	readonly setPerspective?: (perspective: Color) => void;
};

type AnimationState = {
	board: Board;
	keyframeBoards: Board[];
	animationIndex: number;
	animationStarted: boolean;
};

const minWidth = 200;

export default function BoardView({
	board,
	keyframeBoards,
	laserFrame,
	defaultWidth,
	defaultBoardPerspective,
	mouseDownOnTile,
	mouseUpOnTile,
	mouseMoveOnTile,
	floatingPieces,
	ghostPieces,
	maskedPieces,
	highlightedSquares,
	rotationTile,
	hoverOnRotation,
	laser,
	setPerspective,
}: Properties) {
	const [width, setWidth] = useState(defaultWidth);
	const [animationState, setAnimationState] = useState<AnimationState>({
		board,
		keyframeBoards,
		animationIndex: 0,
		animationStarted: false,
	});
	if (animationState.board !== board || animationState.keyframeBoards !== keyframeBoards) {
		setAnimationState({ board, keyframeBoards, animationIndex: 0, animationStarted: false });
	}

	const animationGroupReference = useRef(null);

	const animationTimeMs = 250;
	const [currentBoard, maskedFileRanks, animations] = useMemo(() => {
		const currentBoard = animationState.keyframeBoards[animationState.animationIndex];
		if (currentBoard === undefined) {
			return [animationState.board, new Set<string>(), []];
		}

		const animations = getAnimations(
			currentBoard,
			animationState.keyframeBoards[animationState.animationIndex + 1] ?? animationState.board
		);
		return [
			currentBoard,
			new Set<string>(animations.map((animation) => JSON.stringify(animation.from))),
			animations,
		];
	}, [animationState]);
	const boardElement = useRef<HTMLDivElement>(null);

	// Calculate board dimensions in proportion to width
	const [dragStart, setDragStart] = useState<[number, number] | undefined>(undefined);
	const computeDraggedWidth = useCallback(
		(draggedCoords: [number, number] | undefined) => {
			if (draggedCoords === undefined || dragStart === undefined) return width;
			return Math.max(
				minWidth,
				width + (draggedCoords[0] - dragStart[0]) + (draggedCoords[1] - dragStart[1])
			);
		},
		[dragStart, width]
	);

	const [draggedCoords, startDrag] = useMouseDrag(
		0,
		useCallback(
			(finalDraggedCoords) => {
				setWidth(computeDraggedWidth(finalDraggedCoords));
			},
			[computeDraggedWidth]
		)
	);
	const computedWidth = computeDraggedWidth(draggedCoords);
	const height = (currentBoard.ranks / currentBoard.files) * computedWidth;
	const squareSize = computedWidth / currentBoard.files;
	// Size of pieces relative to squares
	const pieceSize = 0.75;

	const [boardPerspective, setBoardPerspective] = useState(defaultBoardPerspective);
	const transformFileRankCoords = useCallback(
		(fileCoord: number, rankCoord: number): [number, number] =>
			boardPerspective === 'white'
				? [fileCoord, currentBoard.ranks - 1 - rankCoord]
				: [currentBoard.files - 1 - fileCoord, rankCoord],
		[boardPerspective, currentBoard.files, currentBoard.ranks]
	);
	const getMouseEventTile: (event: MouseEvent) => [number, number] = useCallback(
		(event) => {
			const boardRect = event.currentTarget.getBoundingClientRect();
			const [file, rank] = transformFileRankCoords(
				Math.floor((event.clientX - boardRect.x) / squareSize),
				Math.floor((event.clientY - boardRect.y) / squareSize)
			);

			return [
				Math.max(0, Math.min(file, currentBoard.files - 1)),
				Math.max(0, Math.min(rank, currentBoard.ranks - 1)),
			];
		},
		[currentBoard.ranks, currentBoard.files, squareSize, transformFileRankCoords]
	);

	// Helper to check square color, used to color rank/file labels
	const isSquareDark = (fileCoord: number, rankCoord: number) => {
		const [file, rank] = transformFileRankCoords(fileCoord, rankCoord);
		// Bottom right-hand corner for white is always a light square
		return (currentBoard.files - 1 - file + rank) % 2 === 1;
	};

	// Square colors
	const darkColor = 'tan';
	const lightColor = 'beige';

	return (
		<div
			ref={boardElement}
			className="boardContainer"
			style={{ width: computedWidth, height }}
			onMouseDown={useCallback(
				(event: MouseEvent) => {
					if (mouseDownOnTile !== undefined && draggedCoords === undefined) {
						mouseDownOnTile(event, ...getMouseEventTile(event));
					}
				},
				[getMouseEventTile, mouseDownOnTile, draggedCoords]
			)}
			onMouseUp={useCallback(
				(event: MouseEvent) => {
					if (mouseUpOnTile !== undefined) {
						mouseUpOnTile(event, ...getMouseEventTile(event));
					}
				},
				[getMouseEventTile, mouseUpOnTile]
			)}
			onMouseMove={useCallback(
				(event: MouseEvent) => {
					if (mouseMoveOnTile !== undefined) {
						mouseMoveOnTile(event, ...getMouseEventTile(event));
					}
				},
				[getMouseEventTile, mouseMoveOnTile]
			)}
		>
			<BoardBackground
				ranks={currentBoard.ranks}
				files={currentBoard.files}
				boardPerspective={boardPerspective}
				darkColor={darkColor}
				lightColor={lightColor}
			/>
			<div
				className="boardRankLabels"
				style={{ fontSize: `${width / Math.max(currentBoard.files, currentBoard.ranks) / 4}px` }}
				onMouseDown={() => false}
			>
				{forRange(currentBoard.ranks, (rankCoord) => (
					<div
						key={rankCoord}
						style={{
							color: isSquareDark(currentBoard.files - 1, rankCoord) ? lightColor : darkColor,
						}}
					>
						{transformFileRankCoords(0, rankCoord)[1]}
					</div>
				))}
			</div>
			<div
				className="boardFileLabels"
				style={{ fontSize: `${width / Math.max(currentBoard.files, currentBoard.ranks) / 4}px` }}
				onMouseDown={() => false}
			>
				{forRange(currentBoard.files, (fileCoord) => (
					<div
						key={fileCoord}
						style={{
							color: isSquareDark(fileCoord, currentBoard.ranks - 1) ? lightColor : darkColor,
						}}
					>
						{getFileString(transformFileRankCoords(fileCoord, 0)[0])}
					</div>
				))}
			</div>
			<Annotations ranks={currentBoard.ranks} files={currentBoard.files} squareSize={squareSize}>
				<div className="highlights">
					{highlightedSquares === undefined
						? null
						: highlightedSquares.map(({ square, highlighting }) => {
								const [x, y] = transformFileRankCoords(...square);
								return (
									<div
										key={`${square[0]} ${square[1]} ${highlighting}`}
										className={
											highlighting === 'dotted' && board.getSquare(...square).kind !== 'empty'
												? 'dottedBorder'
												: highlighting
										}
										style={{
											left: x * squareSize,
											top: y * squareSize,
											width: squareSize,
											height: squareSize,
										}}
									/>
								);
							})}
				</div>
				{(() => {
					if (rotationTile !== undefined) {
						const { position, validRotations } = rotationTile;
						const piece = currentBoard.getSquare(...position);
						if (piece.kind !== 'empty') {
							const [x, y] = transformFileRankCoords(...position);
							return (
								<RotationPicker
									piece={piece}
									boardPerspective={boardPerspective}
									validRotations={validRotations}
									hoverOnRotation={hoverOnRotation}
									x={x * squareSize}
									y={y * squareSize}
									squareSize={squareSize}
								/>
							);
						}
					}

					return null;
				})()}
			</Annotations>
			<svg
				className="pieces"
				viewBox={`0 0 ${currentBoard.files} ${currentBoard.ranks}`}
				xmlns="http://www.w3.org/2000/svg"
			>
				{
					// Draw all pieces that aren't selected
					([] as any[]).concat(
						...forRange(currentBoard.ranks, (rankCoord) =>
							forRange(currentBoard.files, (fileCoord) => {
								const square = currentBoard.getSquare(fileCoord, rankCoord);
								const [x, y] = transformFileRankCoords(fileCoord, rankCoord);
								return square.kind === 'empty' ||
									maskedFileRanks.has(JSON.stringify([fileCoord, rankCoord])) ||
									maskedPieces?.some(
										([maskedFile, maskedRank]) =>
											maskedFile === fileCoord && maskedRank === rankCoord
									) ? null : (
									<PieceView
										key={`${fileCoord},${rankCoord}`}
										piece={square}
										boardPerspective={boardPerspective}
										centerX={x + 0.5}
										centerY={y + 0.5}
										size={pieceSize}
									/>
								);
							})
						)
					)
				}
				<TransitionGroup component={null}>
					<Transition
						key={`${animationState.board.toFen()} ${animationState.keyframeBoards.map((board) => board.toFen()).join(' ')} ${animationState.animationIndex} `}
						mountOnEnter
						nodeRef={animationGroupReference}
						timeout={{ enter: animationTimeMs }}
						onEnter={() => {
							setAnimationState((animationState) => ({
								...animationState,
								animationStarted: false,
							}));
						}}
						onEntering={() => {
							setAnimationState((animationState) => ({
								...animationState,
								animationStarted: true,
							}));
						}}
						onEntered={() => {
							setAnimationState((animationState) => ({
								...animationState,
								animationIndex:
									animationState.animationIndex < animationState.keyframeBoards.length
										? animationState.animationIndex + 1
										: animationState.animationIndex,
								animationStarted: false,
							}));
						}}
					>
						<g ref={animationGroupReference} className="animationGroup">
							{animations.map((animation) => {
								const piece = animation.piece;
								const newPiece = match([piece, animation])
									.with([{ kind: 'pawn' }, { kind: 'pushRotate' }], ([pawn, animation]) => ({
										...pawn,
										mirrorDirection: rotateOrdinalDirection(
											pawn.mirrorDirection,
											animation.rotation
										),
									}))
									.with([{ kind: 'monarch' }, { kind: 'pushRotate' }], ([monarch, animation]) => ({
										...monarch,
										laserDirection: rotateDirection(monarch.laserDirection, animation.rotation),
									}))
									.otherwise(() => piece);
								const [x, y] = transformFileRankCoords(
									...(animationState.animationStarted && animation.kind === 'pushRotate'
										? animation.to
										: animation.from)
								);
								const opacity = match(animation.kind)
									.with('fadeIn', () => (animationState.animationStarted ? 1 : 0))
									.with('fadeOut', () => (animationState.animationStarted ? 0 : 1))
									.with('pushRotate', () => 1)
									.exhaustive();
								return (
									<PieceView
										key={`${JSON.stringify(animation)}`}
										piece={animationState.animationStarted ? newPiece : piece}
										boardPerspective={boardPerspective}
										centerX={x + 0.5}
										centerY={y + 0.5}
										opacity={opacity}
										size={pieceSize}
									/>
								);
							})}
						</g>
					</Transition>
				</TransitionGroup>
				{
					// Draw ghost pieces
					ghostPieces === undefined
						? null
						: ghostPieces.map(({ piece, position }) => {
								const [x, y] = transformFileRankCoords(...position);
								return (
									<PieceView
										key={`${x},${y}`}
										piece={piece}
										boardPerspective={boardPerspective}
										centerX={x + 0.5}
										centerY={y + 0.5}
										size={pieceSize}
										opacity={0.5}
									/>
								);
							})
				}
				{
					// Now, draw the selected piece (so that it is in foreground)
					floatingPieces === undefined
						? null
						: floatingPieces.map(({ piece, absoluteCoords, id }) => {
								if (boardElement.current !== null) {
									const boardRect = boardElement.current.getBoundingClientRect();
									return (
										<PieceView
											key={id}
											piece={piece}
											boardPerspective={boardPerspective}
											centerX={(absoluteCoords[0] - boardRect.x) / squareSize}
											centerY={(absoluteCoords[1] - boardRect.y) / squareSize}
											size={pieceSize}
										/>
									);
								}

								return null;
							})
				}
			</svg>
			{laser && (laser.isTentative || animationState.animationIndex >= laserFrame) ? (
				<LaserView
					color={laser.color}
					steps={laser.steps}
					isTentative={laser.isTentative}
					start={laser.start}
					files={currentBoard.files}
					ranks={currentBoard.ranks}
					transformFileRankCoords={transformFileRankCoords}
				/>
			) : null}

			<svg
				className="draggableCornerContainer"
				width={squareSize * 0.2}
				height={squareSize * 0.2}
				viewBox="0 0 0.2 0.2"
				xmlns="http://www.w3.org/2000/svg"
			>
				<path strokeWidth={0.01} stroke="black" d="M 0 0.2 L 0.2 0 M 0.1 0.2 L 0.2 0.1" />
				<circle
					className="draggableCorner"
					cx={0.1}
					cy={0.1}
					r={0.15}
					onMouseDown={(event) => {
						setDragStart([event.clientX, event.clientY]);
						startDrag(event);
						event.stopPropagation();
					}}
				/>
			</svg>
			<svg
				className="boardFlipContainer"
				width={squareSize * 0.2}
				height={squareSize * 0.2}
				viewBox="0 0 0.2 0.2"
				xmlns="http://www.w3.org/2000/svg"
			>
				<defs>
					<marker
						id="arrow"
						viewBox="0 0 10 10"
						refX="5"
						refY="5"
						markerWidth="6"
						markerHeight="6"
						orient="auto-start-reverse"
					>
						<path fill="gray" d="M 4 0 L 10 5 L 4 10 z" />
					</marker>
				</defs>
				<path
					strokeWidth={0.05}
					stroke="gray"
					d="M 0 0.2 A 0.2 0.2 0 1 1 0.2 0"
					markerEnd="url(#arrow)"
				/>
				<circle
					className="boardFlipper"
					cx={0}
					cy={0}
					r={0.2}
					onMouseDown={(event) => {
						setBoardPerspective((boardPerspective) => {
							const newPerspective = boardPerspective === 'white' ? 'black' : 'white';
							if (setPerspective) {
								setPerspective(newPerspective);
							}

							return newPerspective;
						});
						event.stopPropagation();
					}}
				/>
			</svg>
		</div>
	);
}
