import assert from 'node:assert';
import { useCallback } from 'react';
import { match } from 'ts-pattern';
import { type Rotation } from '@/types/move';

type Properties = {
	readonly currentAngle: number;
	readonly newAngle: number;
	readonly validRotations: Rotation[];
	readonly viewScale: number;
	readonly hoveredRotation?: Rotation;
	readonly setHover: (rotation?: Rotation) => void;
};

export default function RotationChoice({
	currentAngle,
	newAngle,
	validRotations,
	viewScale,
	hoveredRotation,
	setHover: hoverOnRotation,
}: Properties) {
	const rotation: Rotation = match((newAngle - currentAngle + 360) % 360)
		.with(0, () => 'nullMove')
		.with(90, () => 'R')
		.with(180, () => 'U')
		.with(270, () => 'L')
		.otherwise(() => {
			assert(false);
		});
	const rotationClass = validRotations.includes(rotation) ? 'rotationChoice' : 'invalidChoice';
	const onMouseEnter = useCallback(() => {
		if (hoverOnRotation !== undefined) hoverOnRotation(rotation);
	}, [hoverOnRotation, rotation]);

	const onMouseLeave = useCallback(() => {
		if (hoveredRotation === rotation && hoverOnRotation) {
			hoverOnRotation(undefined);
		}
	}, [hoverOnRotation, hoveredRotation, rotation]);

	const referenceHandler = useCallback(
		// eslint-disable-next-line @typescript-eslint/ban-types
		(reference: SVGUseElement | null) => {
			if (reference?.matches(':hover') && hoverOnRotation) {
				hoverOnRotation(rotation);
			}
		},
		[rotation, hoverOnRotation]
	);

	return (
		<use
			key={newAngle}
			ref={referenceHandler}
			href="#rotationChoice"
			className={rotationClass}
			transform={`rotate(${newAngle} ${viewScale / 2} ${viewScale / 2})`}
			onMouseEnter={onMouseEnter}
			onMouseLeave={onMouseLeave}
		/>
	);
}
