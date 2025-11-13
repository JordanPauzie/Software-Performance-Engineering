import { type Color } from '@/types/board';

type Properties = {
	readonly ranks: number;
	readonly files: number;
	readonly boardPerspective: Color;
	readonly darkColor: string;
	readonly lightColor: string;
};

export default function BoardBackground({
	ranks,
	files,
	boardPerspective,
	darkColor,
	lightColor,
}: Properties) {
	return (
		<svg
			className="board"
			viewBox={`0 .5 ${files} ${ranks}`}
			fill={darkColor}
			stroke={lightColor}
			strokeDasharray="1"
			shapeRendering="crispEdges"
			xmlns="http://www.w3.org/2000/svg"
		>
			<circle r={files + ranks} />
			<path
				transform={
					boardPerspective === 'white' ? `rotate(180, ${files / 2}, ${ranks / 2 + 0.5})` : ''
				}
				d={`M.5.5${Array.from({ length: files }, (_, file) => {
					// Based on approach used in https://upload.wikimedia.org/wikipedia/commons/d/d7/Chessboard480.svg
					// preceding <circle> element sets background in dark square color
					// then, draw a dashed line in zig-zag fashion over this background with light square color to produce alternating light squares
					if (file === 0) {
						return `V${ranks}`;
					}

					return file % 2 === 0 ? `h1V${file === files - 1 ? ranks + 1 : ranks}` : 'h1V1';
				}).join('')}`}
			/>
		</svg>
	);
}
