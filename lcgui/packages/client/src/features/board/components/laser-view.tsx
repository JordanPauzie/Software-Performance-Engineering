import { type Color, type Direction, takeStep } from '@/types/board';
import { type FileRank } from '@/types/move';

type Properties = {
	readonly color: Color;
	readonly steps: Direction[];
	readonly isTentative: boolean;
	readonly start: FileRank;
	readonly files: number;
	readonly ranks: number;
	readonly transformFileRankCoords: (fileCoord: number, rankCoord: number) => FileRank;
};

export default function LaserView({
	color,
	steps,
	isTentative,
	start,
	files,
	ranks,
	transformFileRankCoords,
}: Properties) {
	const whiteLaser = '#FF8000';
	const blackLaser = '#CC66FF';
	const laserPath = [start];
	let currentCoords = start;
	for (const step of steps) {
		currentCoords = takeStep(step, ...currentCoords);
		laserPath.push(currentCoords);
	}

	return (
		<svg className="laser" viewBox={`0 0 ${files} ${ranks}`} xmlns="http://www.w3.org/2000/svg">
			<path
				stroke={color === 'white' ? whiteLaser : blackLaser}
				fill="transparent"
				strokeDasharray={isTentative ? '0.3' : undefined}
				strokeWidth={0.1}
				d={laserPath
					.map(([file, rank], index) => {
						const [fileCoord, rankCoord] = transformFileRankCoords(file, rank);
						return `${index === 0 ? 'M' : 'L'} ${fileCoord + 0.5} ${rankCoord + 0.5}`;
					})

					.join(' ')}
			/>
		</svg>
	);
}
