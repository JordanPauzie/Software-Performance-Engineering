import assert from 'node:assert';
import { type ReactElement, useEffect, useRef, useState } from 'react';
import { match, P } from 'ts-pattern';
import { getMoveString, type Move } from '@/types/move';
import { type AtLeastOne } from '@/utils';

type Properties = {
	readonly transitions: AtLeastOne<Array<[number, Move]>>;
	readonly parents: AtLeastOne<number | undefined>;
	readonly selectedNode: number;
	readonly selectNode: (nodeIndex: number) => void;
};

export default function MoveHistory({
	transitions,
	parents,
	selectedNode,
	selectNode,
}: Properties) {
	useEffect(() => {
		const listener = (event: KeyboardEvent) => {
			if (event.key === 'ArrowLeft') {
				const parent = parents[selectedNode];
				if (parent !== undefined) {
					selectNode(parent);
				}
			} else if (event.key === 'ArrowRight') {
				const mainTransition = transitions[selectedNode]![0];
				if (mainTransition !== undefined) {
					const mainChildIndex = mainTransition[0];
					selectNode(mainChildIndex);
				}
			}
		};

		addEventListener('keydown', listener);
		return () => {
			removeEventListener('keydown', listener);
		};
	}, [selectedNode, selectNode, transitions, parents]);

	function makeMainLineListing(
		whiteMoveEntry: ReactElement | undefined,
		blackMoveEntry: ReactElement | undefined,
		insertPlaceholders: boolean,
		turn: number
	): ReactElement {
		const placeholder = insertPlaceholders ? (
			<span className="moveEntry placeholder">...</span>
		) : null;
		assert(insertPlaceholders || whiteMoveEntry !== undefined);
		return (
			<span key={`main${turn} ${whiteMoveEntry === undefined ? 1 : 0}`} className="mainLineListing">
				<span className="mainLineTurn">{turn}</span>
				{whiteMoveEntry ?? placeholder} {blackMoveEntry ?? placeholder}
			</span>
		);
	}

	const [selectedMoveReference, setSelectedMoveReference] = useState<HTMLElement | undefined>();
	const moveHistoryReference = useRef<HTMLDivElement>(null);
	const distanceBeforeScroll = 50;
	if (selectedMoveReference !== undefined && moveHistoryReference.current !== null) {
		const bottomScrollDistance =
			selectedMoveReference.getBoundingClientRect().bottom -
			moveHistoryReference.current.getBoundingClientRect().bottom +
			distanceBeforeScroll;
		const topScrollDistance =
			moveHistoryReference.current.getBoundingClientRect().top -
			selectedMoveReference.getBoundingClientRect().top +
			distanceBeforeScroll;
		if (bottomScrollDistance > 0) {
			moveHistoryReference.current.scrollBy(0, bottomScrollDistance);
		} else if (topScrollDistance > 0) {
			moveHistoryReference.current.scrollBy(0, -topScrollDistance);
		}
	}

	function makeMoveEntry(
		move: Move,
		nodeIndex: number,
		entryType: 'mainLine' | 'variation'
	): ReactElement {
		return (
			<span
				ref={(reference) => {
					if (nodeIndex === selectedNode && reference !== null) {
						setSelectedMoveReference(reference);
					}
				}}
				key={nodeIndex}
				className={`moveEntry ${entryType} ${nodeIndex === selectedNode ? 'selected' : ''}`}
				onClick={() => {
					selectNode(nodeIndex);
				}}
			>
				{getMoveString(move)}
			</span>
		);
	}

	type MainLineInfo = {
		ungroupedMoves: ReactElement[];
		moveGroups: ReactElement[];
	};

	type VariationInfo = MainLineInfo & {
		hasInlineSubvariation: boolean;
	};

	function makeVariationTurnLabel(parentPly: number): ReactElement {
		return (
			<span key={`turn${parentPly}`} className="variationTurn">
				{Math.floor(parentPly / 2 + 1)}
				{parentPly % 2 === 0 ? '.' : '...'}
			</span>
		);
	}

	function makeVariation(
		{ ungroupedMoves, moveGroups }: VariationInfo,
		parentPly: number,
		childNodeIndex: number
	): ReactElement {
		return (
			<div key={childNodeIndex} className="variationSubtree">
				{ungroupedMoves.length === 0 ? null : (
					<div className="beforeVariationListing">
						<span className="variationListing">
							{makeVariationTurnLabel(parentPly)}
							{ungroupedMoves}
						</span>
					</div>
				)}
				{moveGroups}
			</div>
		);
	}

	function makeSubvariation(ungroupedMoves: ReactElement[], parentPly: number): ReactElement {
		return (
			<span key="subvar" className="subvariationListing">
				({makeVariationTurnLabel(parentPly)}
				{ungroupedMoves})
			</span>
		);
	}

	function renderVariation(nodeIndex: number, move: Move, ply: number): VariationInfo {
		const moveEntry = makeMoveEntry(move, nodeIndex, 'variation');
		return match(transitions[nodeIndex]!)
			.with([P._], ([[mainChildIndex, moveToMainChild]]) => {
				const mainChildResult = renderVariation(mainChildIndex, moveToMainChild, ply + 1);
				return {
					...mainChildResult,
					ungroupedMoves: [
						moveEntry,
						...(ply % 2 === 0 ? [makeVariationTurnLabel(ply)] : []),
						...mainChildResult.ungroupedMoves,
					],
				};
			})
			.with(
				[P._, P._],
				([[mainChildIndex, moveToMainChild], [subvariationIndex, moveToSubvariation]]) => {
					const mainChildResult = renderVariation(mainChildIndex, moveToMainChild, ply + 1);
					const subvariationResult = renderVariation(
						subvariationIndex,
						moveToSubvariation,
						ply + 1
					);
					if (
						!subvariationResult.hasInlineSubvariation &&
						subvariationResult.moveGroups.length === 0
					) {
						return {
							ungroupedMoves: [
								moveEntry,
								...(ply % 2 === 0 ? [makeVariationTurnLabel(ply)] : []),
								mainChildResult.ungroupedMoves[0]!,
								makeSubvariation(subvariationResult.ungroupedMoves, ply),
								...mainChildResult.ungroupedMoves.slice(1),
							],
							moveGroups: mainChildResult.moveGroups,
							hasInlineSubvariation: true,
						};
					}

					return {
						ungroupedMoves: [moveEntry],
						moveGroups: [
							makeVariation(mainChildResult, ply, mainChildIndex),
							makeVariation(subvariationResult, ply, subvariationIndex),
						],
						hasInlineSubvariation: false,
					};
				}
			)
			.otherwise((subvariations) => ({
				ungroupedMoves: [moveEntry],
				moveGroups: subvariations.map(([subvariationIndex, moveToSubvariation]) =>
					makeVariation(
						renderVariation(subvariationIndex, moveToSubvariation, ply + 1),
						ply,
						subvariationIndex
					)
				),
				hasInlineSubvariation: false,
			}));
	}

	function renderMainLine(nodeIndex: number, move: Move | undefined, ply: number): MainLineInfo {
		const ungroupedMoves = move === undefined ? [] : [makeMoveEntry(move, nodeIndex, 'mainLine')];
		let nextMainLineMove;
		const mainLineMoveGroups = [];
		const variationMoveGroups = [];
		for (const [edgeIndex, [childIndex, moveToChild]] of transitions[nodeIndex]!.entries()) {
			if (edgeIndex === 0) {
				const childResult = renderMainLine(childIndex, moveToChild, ply + 1);
				const turn = Math.floor(ply / 2) + 1;
				if (ply % 2 === 0) {
					const [whiteMove, blackMove] = childResult.ungroupedMoves;
					if (transitions[nodeIndex]!.length === 1) {
						nextMainLineMove = makeMainLineListing(whiteMove, blackMove, false, turn);
					} else if (blackMove === undefined) {
						nextMainLineMove = makeMainLineListing(whiteMove, undefined, false, turn);
					} else {
						nextMainLineMove = makeMainLineListing(whiteMove, undefined, true, turn);
						mainLineMoveGroups.push(makeMainLineListing(undefined, blackMove, true, turn));
					}
				} else {
					ungroupedMoves.push(...childResult.ungroupedMoves);
				}

				mainLineMoveGroups.push(...childResult.moveGroups);
			} else {
				variationMoveGroups.push(
					makeVariation(renderVariation(childIndex, moveToChild, ply + 1), ply, childIndex)
				);
			}
		}

		const moveGroups = [];
		if (nextMainLineMove !== undefined) moveGroups.push(nextMainLineMove);
		if (variationMoveGroups.length > 0)
			moveGroups.push(
				makeVariation(
					{ ungroupedMoves: [], hasInlineSubvariation: false, moveGroups: variationMoveGroups },
					ply,
					-1
				)
			);
		moveGroups.push(...mainLineMoveGroups);

		return {
			ungroupedMoves,
			moveGroups,
		};
	}

	const rootResult = renderMainLine(0, undefined, 0);
	return (
		<div ref={moveHistoryReference} className="moveHistory">
			{rootResult.moveGroups}
		</div>
	);
}
