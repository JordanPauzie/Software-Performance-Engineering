import { type MouseEventHandler, useEffect, useState, useCallback } from 'react';

export function useMouseDrag(
	button: number,
	onDragReset: (finalDraggedCoords: [number, number] | undefined) => void
): [[number, number] | undefined, MouseEventHandler] {
	const [draggedCoords, setDraggedCoords] = useState<undefined | [number, number]>(undefined);
	// Add mouse handlers to detect when mouse is clicked and dragged
	useEffect(() => {
		const mouseMoveHandler = (event: MouseEvent) => {
			setDraggedCoords((draggedCoords) =>
				draggedCoords === undefined ? undefined : [event.clientX, event.clientY]
			);
		};

		const mouseUpHandler = (event: MouseEvent) => {
			if (event.button === button) {
				setDraggedCoords((draggedCoords) => {
					onDragReset(draggedCoords);
					return undefined;
				});
			}
		};

		document.addEventListener('mousemove', mouseMoveHandler);
		document.addEventListener('mouseup', mouseUpHandler);
		return () => {
			document.removeEventListener('mousemove', mouseMoveHandler);
			document.removeEventListener('mouseup', mouseUpHandler);
		};
	}, [onDragReset, setDraggedCoords, button]);
	const startDrag: MouseEventHandler = useCallback((event) => {
		setDraggedCoords([event.clientX, event.clientY]);
		event.preventDefault();
	}, []);

	return [draggedCoords, startDrag];
}
