import { match, P } from 'ts-pattern';
import { useCallback, useState } from 'react';
import RotationChoice from './rotation-choice';
import { type Color, type Piece } from '@/types/board';
import { type Rotation } from '@/types/move';

type Properties = {
	piece: Piece;
	boardPerspective: Color;
	validRotations: Rotation[];
	hoverOnRotation?: (rotation?: Rotation) => void;
	x: number;
	y: number;
	squareSize: number;
};

export default function RotationPicker({
	piece,
	boardPerspective,
	validRotations,
	hoverOnRotation,
	x,
	y,
	squareSize,
}: Properties) {
	const [hoveredRotation, setHoveredRotation] = useState<Rotation | undefined>(undefined);
	// For hover to work properly, view box must be large
	const viewScale = 600;
	const insetArrowScale = 0.9;
	const boardPerspectiveOffset = match(boardPerspective)
		.with('black', () => 180)
		.with('white', () => 0)
		.exhaustive();
	const setHover = useCallback(
		(rotation?: Rotation) => {
			setHoveredRotation(rotation);
			if (hoverOnRotation) hoverOnRotation(rotation);
		},
		[hoverOnRotation]
	);

	return match(piece)
		.with({ kind: 'monarch', laserDirection: P.select() }, (laserDirection) => {
			const currentAngle =
				(match(laserDirection)
					.with('north', () => 0)
					.with('east', () => 90)
					.with('south', () => 180)
					.with('west', () => 270)
					.exhaustive() +
					boardPerspectiveOffset) %
				360;
			return (
				<div
					style={{ width: squareSize, height: squareSize, position: 'absolute', left: x, top: y }}
				>
					<svg viewBox={`0 0 ${viewScale} ${viewScale}`} xmlns="http://www.w3.org/2000/svg">
						<defs>
							<g id="rotationChoice">
								<polygon points={`${0},${0} ${viewScale / 2},${viewScale / 2} ${viewScale},${0}`} />
								<polygon
									className="rotationInset"
									points={[
										`${viewScale / 2},${(viewScale * (1 - insetArrowScale)) / 6}`,
										`${(viewScale * (2 - insetArrowScale)) / 4},${(viewScale * (2 + insetArrowScale)) / 12}`,
										`${(viewScale * (2 + insetArrowScale)) / 4},${(viewScale * (2 + insetArrowScale)) / 12}`,
									].join(' ')}
								/>
							</g>
						</defs>
						{[0, 90, 180, 270].map((newAngle) => (
							<RotationChoice
								key={newAngle}
								currentAngle={currentAngle}
								newAngle={newAngle}
								validRotations={validRotations}
								viewScale={viewScale}
								hoveredRotation={hoveredRotation}
								setHover={setHover}
							/>
						))}
					</svg>
				</div>
			);
		})
		.with({ kind: 'pawn', mirrorDirection: P.select() }, (mirrorDirection) => {
			const currentAngle =
				(match(mirrorDirection)
					.with(['north', 'east'], () => 0)
					.with(['south', 'east'], () => 90)
					.with(['south', 'west'], () => 180)
					.with(['north', 'west'], () => 270)
					.exhaustive() +
					boardPerspectiveOffset) %
				360;
			const margin = (viewScale / 4) * (1 - insetArrowScale);
			return (
				<div
					style={{ width: squareSize, height: squareSize, position: 'absolute', left: x, top: y }}
				>
					<svg viewBox={`0 0 ${viewScale} ${viewScale}`} xmlns="http://www.w3.org/2000/svg">
						<defs>
							<g id="rotationChoice">
								<polygon
									points={`${viewScale / 2},${0} ${viewScale},${0} ${viewScale},${viewScale / 2} ${viewScale / 2},${viewScale / 2}`}
								/>
								<polygon
									className="rotationInset"
									points={[
										`${viewScale / 2 + margin},${margin}`,
										`${viewScale - margin},${margin}`,
										`${viewScale - margin},${viewScale / 2 - margin}`,
									].join(' ')}
								/>
							</g>
						</defs>
						{[0, 90, 180, 270].map((newAngle) => (
							<RotationChoice
								key={newAngle}
								currentAngle={currentAngle}
								newAngle={newAngle}
								validRotations={validRotations}
								viewScale={viewScale}
								hoveredRotation={hoveredRotation}
								setHover={setHover}
							/>
						))}
					</svg>
				</div>
			);
		})
		.exhaustive();
}
