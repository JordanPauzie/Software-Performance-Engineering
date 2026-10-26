import {
	useCallback,
	useState,
	type MouseEventHandler,
	type MouseEvent,
	useRef,
	useEffect,
	type PropsWithChildren,
} from 'react';
import { useMouseDrag } from '@/hooks/use-mouse-drag';
import { type FileRank } from '@/types/move';

type Properties = {
	readonly squareSize: number;
	readonly ranks: number;
	readonly files: number;
};

export default function Annotations({
	children,
	squareSize,
	ranks,
	files,
}: PropsWithChildren<Properties>) {
	type Annotation =
		| { kind: 'point'; coords: FileRank }
		| { kind: 'arrow'; from: FileRank; to: FileRank };
	const [startPoint, setStartPoint] = useState<FileRank | undefined>(undefined);
	const [shiftHeld, setShiftHeld] = useState<boolean>(false);
	useEffect(() => {
		const downListener = (event: KeyboardEvent) => {
			if (event.key === 'Shift') {
				setShiftHeld(true);
			}
		};

		document.addEventListener('keydown', downListener);
		const upListener = (event: KeyboardEvent) => {
			if (event.key === 'Shift') {
				setShiftHeld(false);
			}
		};

		document.addEventListener('keyup', upListener);
		return () => {
			document.removeEventListener('keydown', downListener);
			document.removeEventListener('keyup', upListener);
		};
	}, []);
	const onResetDrag = useCallback(() => {
		setStartPoint(undefined);
	}, [setStartPoint]);
	const [leftClickDraggedCoords, startLeftClickDrag] = useMouseDrag(0, onResetDrag);
	const [rightClickDraggedCoords, startRightClickDrag] = useMouseDrag(2, onResetDrag);
	const draggedCoords = leftClickDraggedCoords ?? rightClickDraggedCoords;
	const [annotations, setAnnotations] = useState<Annotation[]>([]);
	const boardElement = useRef<HTMLDivElement>(null);
	const getMouseFileRank = useCallback(
		(event: MouseEvent): FileRank => {
			const boardRect = event.currentTarget.getBoundingClientRect();
			const file = Math.max(
				0,
				Math.min(Math.floor((event.clientX - boardRect.x) / squareSize), files - 1)
			);
			const rank = Math.max(
				0,
				Math.min(Math.floor((event.clientY - boardRect.y) / squareSize), ranks - 1)
			);
			return [file, rank];
		},
		[squareSize, ranks, files]
	);
	const beginAnnotation: MouseEventHandler = useCallback(
		(event) => {
			if (event.button === 2) {
				if (leftClickDraggedCoords === undefined) {
					startRightClickDrag(event);
					setStartPoint(getMouseFileRank(event));
				}

				event.preventDefault();
			} else if (event.button === 0) {
				if (shiftHeld) {
					if (rightClickDraggedCoords === undefined) {
						startLeftClickDrag(event);
						setStartPoint(getMouseFileRank(event));
					}
				} else {
					setAnnotations([]);
				}
			}
		},
		[
			getMouseFileRank,
			leftClickDraggedCoords,
			rightClickDraggedCoords,
			startLeftClickDrag,
			startRightClickDrag,
			shiftHeld,
		]
	);
	const makeAnnotation = useCallback(
		(start: FileRank, end: FileRank): Annotation =>
			start[0] === end[0] && start[1] === end[1]
				? { kind: 'point', coords: end }
				: { kind: 'arrow', from: start, to: end },
		[]
	);
	const finishAnnotation: MouseEventHandler = useCallback(
		(event) => {
			if (
				((event.button === 2 && rightClickDraggedCoords !== undefined) ||
					(event.button === 0 && leftClickDraggedCoords !== undefined)) &&
				startPoint !== undefined
			) {
				event.preventDefault();
				const fileRank = getMouseFileRank(event);
				setAnnotations((annotations) => {
					const newAnnotation: Annotation = makeAnnotation(startPoint, fileRank);
					const matchingAnnotation = (annotation: Annotation) => {
						if (annotation.kind === 'point') {
							return (
								newAnnotation.kind === 'point' &&
								newAnnotation.coords[0] === annotation.coords[0] &&
								newAnnotation.coords[1] === annotation.coords[1]
							);
						}

						return (
							newAnnotation.kind === 'arrow' &&
							newAnnotation.from[0] === annotation.from[0] &&
							newAnnotation.from[1] === annotation.from[1] &&
							newAnnotation.to[0] === annotation.to[0] &&
							newAnnotation.to[1] === annotation.to[1]
						);
					};

					if (!annotations.some((annotation) => matchingAnnotation(annotation))) {
						return annotations.concat([newAnnotation]);
					}

					return annotations.filter((annotation) => !matchingAnnotation(annotation));
				});
			}
		},
		[makeAnnotation, startPoint, getMouseFileRank, leftClickDraggedCoords, rightClickDraggedCoords]
	);

	const arrowHeadSize = 0.6;
	const dotStrokeWidth = 0.05;
	const arrowStrokeWidth = 0.2;

	const makeAnnotationElement = (annotation: Annotation) => {
		if (annotation.kind === 'point') {
			return (
				<circle
					cx={annotation.coords[0] + 0.5}
					cy={annotation.coords[1] + 0.5}
					r={0.5}
					strokeWidth={dotStrokeWidth}
					fill="none"
				/>
			);
		}

		const dxPath = annotation.to[0] - annotation.from[0];
		const dyPath = annotation.to[1] - annotation.from[1];
		const pathLength = Math.hypot(dxPath, dyPath);
		const pathUnitX = dxPath / pathLength;
		const pathUnitY = dyPath / pathLength;
		const truncatedX = annotation.to[0] + 0.5 - pathUnitX * arrowHeadSize;
		const truncatedY = annotation.to[1] + 0.5 - pathUnitY * arrowHeadSize;
		return (
			<path
				strokeWidth={arrowStrokeWidth}
				d={`M ${annotation.from[0] + 0.5} ${annotation.from[1] + 0.5} L ${truncatedX} ${truncatedY}`}
				markerEnd="url(#triangle)"
			/>
		);
	};

	return (
		<div
			ref={boardElement}
			className="annotationContainer"
			onMouseDown={beginAnnotation}
			onMouseUp={finishAnnotation}
			onContextMenu={(event) => {
				event.preventDefault();
			}}
		>
			{children}
			<svg
				className="annotations"
				viewBox={`0 0 ${files} ${ranks}`}
				xmlns="http://www.w3.org/2000/svg"
			>
				<defs>
					<marker
						id="triangle"
						viewBox={`0 0 ${arrowHeadSize} ${arrowHeadSize}`}
						refX="0"
						refY={arrowHeadSize / 2}
						markerUnits="userSpaceOnUse"
						markerWidth={arrowHeadSize}
						markerHeight={arrowHeadSize}
						orient="auto"
					>
						<path
							d={`M 0 0 L ${arrowHeadSize} ${arrowHeadSize / 2} L 0 ${arrowHeadSize} z`}
							className="arrowheadAnnotation"
						/>
					</marker>
				</defs>
				{annotations.map((annotation) => makeAnnotationElement(annotation))}
				{(() => {
					if (
						draggedCoords !== undefined &&
						boardElement.current !== null &&
						startPoint !== undefined
					) {
						const boardRect = boardElement.current.getBoundingClientRect();
						const file = Math.max(
							0,
							Math.min(Math.floor((draggedCoords[0] - boardRect.x) / squareSize), files - 1)
						);
						const rank = Math.max(
							0,
							Math.min(Math.floor((draggedCoords[1] - boardRect.y) / squareSize), ranks - 1)
						);
						return makeAnnotationElement(makeAnnotation(startPoint, [file, rank]));
					}

					return null;
				})()}
			</svg>
		</div>
	);
}
