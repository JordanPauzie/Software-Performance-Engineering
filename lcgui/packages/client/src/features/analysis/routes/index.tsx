import { useEffect, useState } from 'react';
import AnalysisBar from '../components/analysis-bar';
import PgnInput from '../components/pgn-input';
import { type Board, fenToBoard } from '@/types/board';
import { moveEq, type Move } from '@/types/move';
import GameBoard from '@/features/game-client/components/game-board';
import { getStartFen, getValidMoves, type ParsedMoveInfo } from '@/features/game-client/api/client';
import { type AtLeastOne } from '@/utils';
import MoveHistory from '@/features/move-history/components/move-history';
import FenInput from '@/features/board/components/fen-input';

export default function Analysis() {
	type MoveTree = {
		boards: AtLeastOne<Board>;
		keyframeBoards: Board[];
		laserFrame: number;
		transitions: AtLeastOne<Array<[number, Move]>>;
		parents: AtLeastOne<number | undefined>;
		parsedMoveInfos: AtLeastOne<ParsedMoveInfo | undefined>;
		currentNodeIndex: number;
	};
	const [moveTree, setMoveTree] = useState<MoveTree | undefined>(undefined);
	useEffect(() => {
		return getStartFen((startFen) => {
			setMoveTree({
				boards: [fenToBoard(startFen)!],
				keyframeBoards: [],
				laserFrame: 0,
				transitions: [[]],
				parents: [undefined],
				parsedMoveInfos: [undefined],
				currentNodeIndex: 0,
			});
		});
	}, []);
	return (
		<div className="analysis">
			{moveTree === undefined ? (
				<div className="analysisLoading">Loading...</div>
			) : (
				<>
					<div className="analysisBody">
						<div className="analysisBoard">
							<GameBoard
								board={moveTree.boards[moveTree.currentNodeIndex]!}
								keyframeBoards={moveTree.keyframeBoards}
								laserFrame={moveTree.laserFrame}
								lastMove={moveTree.parsedMoveInfos[moveTree.currentNodeIndex]}
								selectMove={(parsedMoveInfo: ParsedMoveInfo) => {
									setMoveTree((moveTree) =>
										moveTree === undefined
											? undefined
											: {
													boards: [...moveTree.boards, parsedMoveInfo.endState],
													keyframeBoards: [
														moveTree.boards[moveTree.currentNodeIndex]!,
														...parsedMoveInfo.keyframeBoards,
													],
													laserFrame: parsedMoveInfo.keyframeBoards.length,
													transitions: moveTree.transitions
														.map((children, nodeIndex) =>
															nodeIndex === moveTree.currentNodeIndex
																? children.concat([[moveTree.boards.length, parsedMoveInfo.move]])
																: children
														)
														.concat([[]]) as AtLeastOne<Array<[number, Move]>>,
													parents: [...moveTree.parents, moveTree.currentNodeIndex],
													parsedMoveInfos: [...moveTree.parsedMoveInfos, parsedMoveInfo],
													currentNodeIndex: moveTree.boards.length,
												}
									);
								}}
								playableColors={['white', 'black']}
								defaultBoardPerspective="white"
							/>
						</div>
						<div className="analysisSidebar">
							<AnalysisBar board={moveTree.boards[moveTree.currentNodeIndex]!} />
							<MoveHistory
								transitions={moveTree.transitions}
								parents={moveTree.parents}
								selectedNode={moveTree.currentNodeIndex}
								selectNode={(nodeIndex) => {
									const [keyframeBoards, laserFrame] = (() => {
										if (nodeIndex === moveTree.parents[moveTree.currentNodeIndex]) {
											const keyframes = [
												moveTree.boards[moveTree.currentNodeIndex]!,
												...(moveTree.parsedMoveInfos[moveTree.currentNodeIndex]?.keyframeBoards
													.slice()
													.reverse() ?? []),
											];
											return [keyframes, keyframes.length];
										}

										for (const [childIndex] of moveTree.transitions[moveTree.currentNodeIndex]!) {
											if (childIndex === nodeIndex) {
												const keyframes = [
													moveTree.boards[moveTree.currentNodeIndex]!,
													...(moveTree.parsedMoveInfos[nodeIndex]?.keyframeBoards ?? []),
												];
												return [keyframes, keyframes.length - 1];
											}
										}

										return [[moveTree.boards[moveTree.currentNodeIndex]!], 1];
									})();
									setMoveTree((moveTree) =>
										moveTree === undefined
											? undefined
											: {
													...moveTree,
													currentNodeIndex: nodeIndex,
													keyframeBoards,
													laserFrame,
												}
									);
								}}
							/>
						</div>
					</div>

					<div className="gameInputs">
						<FenInput
							board={moveTree.boards[moveTree.currentNodeIndex]!}
							setBoard={(board) => {
								setMoveTree({
									boards: [board],
									keyframeBoards: [],
									laserFrame: 0,
									transitions: [[]],
									parents: [undefined],
									parsedMoveInfos: [undefined],
									currentNodeIndex: 0,
								});
							}}
						/>
						<PgnInput
							transitions={moveTree.transitions}
							setTransitions={(transitions, parents) => {
								getStartFen((startFen) => {
									const startBoard = fenToBoard(startFen)!;
									const boards = [startBoard];
									const parsedMoveInfos: Array<ParsedMoveInfo | undefined> = transitions.map(
										() => undefined
									);
									const recurse = (nodeIndex: number) => {
										if (nodeIndex === transitions.length) {
											setMoveTree({
												boards: boards as AtLeastOne<Board>,
												keyframeBoards: [],
												laserFrame: 0,
												transitions,
												parents,
												parsedMoveInfos: parsedMoveInfos as AtLeastOne<ParsedMoveInfo | undefined>,
												currentNodeIndex: 0,
											});
											return;
										}

										const parentIndex = parents[nodeIndex]!;
										const parentBoard = boards[parentIndex]!;
										const edgeToNode = transitions[parentIndex]?.find(
											(edge) => edge[0] === nodeIndex
										);
										if (edgeToNode !== undefined) {
											getValidMoves(parentBoard, (validMoveInfos) => {
												const validMoveInfo = validMoveInfos.find((validMoveInfo) =>
													moveEq(validMoveInfo.move, edgeToNode[1])
												);
												if (validMoveInfo !== undefined) {
													parsedMoveInfos[nodeIndex] = validMoveInfo;
													boards[nodeIndex] = validMoveInfo.endState;
													recurse(nodeIndex + 1);
												}
											});
										}
									};

									recurse(1);
								});
							}}
						/>
					</div>
				</>
			)}
		</div>
	);
}
