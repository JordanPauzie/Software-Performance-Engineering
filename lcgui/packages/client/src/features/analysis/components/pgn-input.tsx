/* eslint-disable complexity */
import { useCallback, useEffect, useRef, useState } from 'react';
import { getMoveString, parseMoveString, type Move } from '@/types/move';
import { type AtLeastOne } from '@/utils';

type Properties = {
	readonly transitions: AtLeastOne<Array<[number, Move]>>;
	readonly setTransitions?: (
		transitions: AtLeastOne<Array<[number, Move]>>,
		parents: AtLeastOne<number | undefined>
	) => void;
};

export default function PgnInput({ transitions, setTransitions }: Properties) {
	const [pgnError, setPgnError] = useState(false);
	const pgnTextbox = useRef<HTMLTextAreaElement>(null);

	const toPgn = useCallback((transitions: AtLeastOne<Array<[number, Move]>>) => {
		const recurse = (nodeIndex: number, hasSiblings: boolean, ply: number): string => {
			const turn = `${Math.floor(ply / 2) + 1}${ply % 2 === 0 ? '.' : '...'} `;
			let head = '';
			const variations = [];
			let tail = '';
			for (const [edgeIndex, [childIndex, moveToChild]] of transitions[nodeIndex]!.entries()) {
				const moveString = getMoveString(moveToChild);
				if (edgeIndex > 0) {
					variations.push(`(${turn}${moveString} ${recurse(childIndex, false, ply + 1)})`);
				} else {
					head = hasSiblings || ply % 2 === 0 ? `${turn}${moveString}` : moveString;
					tail = recurse(childIndex, transitions[nodeIndex]!.length !== 1, ply + 1);
				}
			}

			return [head, ...variations, ...(tail === '' ? [] : [tail])].join(' ');
		};

		return recurse(0, false, 0);
	}, []);
	const fromPgn = useCallback((pgn: string) => {
		const transitions: Array<Array<[number, Move]>> = [[]];
		const parents: Array<undefined | number> = [undefined];
		const stack = [0];
		let insideComment = false;
		let moveString = '';
		const tryParseMove = () => {
			if (moveString === '') return true;
			try {
				const move = parseMoveString(moveString);
				const parent = stack.pop();
				if (parent === undefined) return false;
				const nodeIndex = transitions.length;
				transitions.push([]);
				transitions[parent]!.push([nodeIndex, move]);
				parents[nodeIndex] = parent;
				moveString = '';
				stack.push(nodeIndex);
				return true;
			} catch {
				return false;
			}
		};

		for (const character of pgn) {
			if (character === '{') {
				insideComment = true;
				if (!tryParseMove()) return undefined;
			} else if (character === '}') {
				insideComment = false;
			} else if (insideComment) {
				continue;
			} else if (character === '.') {
				if (moveString !== '' && Number.isNaN(Number.parseInt(moveString, 10))) {
					return undefined;
				}

				moveString = '';
			} else if (/\s/.test(character)) {
				if (!tryParseMove()) return undefined;
			} else if (character === '(') {
				if (!tryParseMove()) return undefined;
				const variationParent = parents[stack.at(-1)!];
				if (variationParent === undefined) return undefined;
				stack.push(variationParent);
			} else if (character === ')') {
				if (!tryParseMove()) return undefined;
				stack.pop();
				if (stack.length === 0) return undefined;
			} else {
				moveString += character;
			}
		}

		if (moveString !== '' && !tryParseMove()) return undefined;
		if (insideComment || stack.length !== 1) return undefined;

		return [
			transitions as AtLeastOne<Array<[number, Move]>>,
			parents as AtLeastOne<number | undefined>,
		] as const;
	}, []);
	useEffect(() => {
		if (pgnTextbox.current !== null) {
			pgnTextbox.current.value = toPgn(transitions);
		}
	}, [transitions, toPgn]);

	return (
		<div className="pgn">
			<span>PGN: </span>
			<textarea
				ref={pgnTextbox}
				name="pgn"
				cols={40}
				rows={4}
				className={`pgnInput${pgnError ? ' error' : ''}`}
			/>
			{setTransitions === undefined ? null : (
				<span
					className="importPgn"
					onClick={() => {
						if (pgnTextbox.current !== null) {
							const pgnParseResult = fromPgn(pgnTextbox.current.value);
							if (pgnParseResult === undefined) {
								setPgnError(true);
							} else {
								setTransitions(...pgnParseResult);
								setPgnError(false);
							}
						}
					}}
				>
					Import
				</span>
			)}
		</div>
	);
}
