import { useEffect, useRef, useState } from 'react';
import { getAnalysis } from '@/features/game-client/api/client';
import { getMoveString, type Move } from '@/types/move';
import { type Board } from '@/types/board';

type Properties = {
	readonly board: Board;
};

const defaultDepth = 8;
const minDepth = 1;
const maxDepth = 20;

export default function AnalysisBar({ board }: Properties) {
	const [analysisOn, setAnalysisOn] = useState(false);
	const [analysisResult, setAnalysisResult] = useState<[Move[], number] | undefined>(undefined);
	const [analysisDepth, setAnalysisDepth] = useState<number>(defaultDepth);
	useEffect(() => {
		setAnalysisResult(undefined);
		if (analysisOn) {
			return getAnalysis(board, analysisDepth, (pv, score) => {
				setAnalysisResult([pv, score]);
			});
		}

		return () => undefined;
	}, [analysisOn, setAnalysisResult, board, analysisDepth]);
	const depthInputReference = useRef<HTMLInputElement>(null);

	return (
		<div className="analysisBar">
			<div className="analysisControls">
				<span
					className={`analysisSwitch ${analysisOn ? 'on' : 'off'}`}
					onClick={() => {
						setAnalysisOn((analysisOn) => !analysisOn);
					}}
				>
					{analysisOn ? '✔' : '✖'}
				</span>
				<span className="analysisScore">
					{analysisResult === undefined
						? ''
						: (() => {
								const score = analysisResult[1] * (board.playerToMove === 'white' ? 1 : -1);
								return `${score > 0 ? '+' : ''}${score / 100}`;
							})()}
				</span>
				<div className="depthControl">
					<input
						ref={depthInputReference}
						className="depthInput"
						type="range"
						min={minDepth}
						max={maxDepth}
						defaultValue={defaultDepth}
						onInput={() => {
							const value = depthInputReference.current?.value;
							if (value !== undefined) {
								setAnalysisDepth(Number.parseInt(value, 10));
							}
						}}
					/>
					<span className="depthLabel">{`Depth (${analysisDepth})`} </span>
				</div>
			</div>
			{analysisResult === undefined ? null : (
				<div className="principalVariation">
					{analysisResult[0].map((move) => getMoveString(move)).join(' ')}
				</div>
			)}
		</div>
	);
}
