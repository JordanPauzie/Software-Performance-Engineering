import { useEffect, useRef, useState } from 'react';
import { type Board, fenToBoard } from '@/types/board';

type Properties = {
	readonly board: Board;
	readonly setBoard: (board: Board) => void;
};

export default function FenInput({ board, setBoard }: Properties) {
	const [fenError, setFenError] = useState(false);
	const fenTextbox = useRef<HTMLTextAreaElement>(null);

	useEffect(() => {
		if (fenTextbox.current !== null) {
			fenTextbox.current.value = board.toFen();
		}
	}, [board]);

	return (
		<div className="skittlesOutput">
			<span>FEN: </span>
			<textarea
				ref={fenTextbox}
				name="fen"
				cols={40}
				rows={2}
				className={`fenInput${fenError ? ' error' : ''}`}
				onChange={(event) => {
					const newBoard = fenToBoard(event.currentTarget.value);
					if (newBoard === undefined) {
						setFenError(true);
					} else {
						setBoard(newBoard);
						setFenError(false);
					}
				}}
			/>
		</div>
	);
}
