import { useState } from 'react';
import { useForm } from 'react-hook-form';
import { type GameControl } from '../api/client';
import Game from '../components/game';
import { type Color } from '@/types/board';

type TimeForm = {
	whiteLimit: number;
	whiteIncrement: number;
	blackLimit: number;
	blackIncrement: number;
};

type DepthForm = {
	depth: number;
};

export default function GamePage() {
	const [selectingTimeLimits, setSelectingTimeLimits] = useState(true);
	const [playerColor, setPlayerColor] = useState<Color>('white');
	const {
		register: registerTimeForm,
		handleSubmit: handleSubmitTimeForm,
		formState: { errors: timeFormErrors },
	} = useForm<TimeForm>({
		defaultValues: { whiteLimit: 600, whiteIncrement: 0, blackLimit: 600, blackIncrement: 0 },
	});
	const {
		register: registerDepthForm,
		handleSubmit: handleSubmitDepthForm,
		formState: { errors: depthFormErrors },
	} = useForm<DepthForm>({ defaultValues: { depth: 7 } });
	const [gameControl, setGameControl] = useState<GameControl | undefined>(undefined);

	return gameControl === undefined ? (
		<div className="configGame">
			<div className="configGameTitle">Play against your bot!</div>
			<div className="colorSelector">
				<span className="colorSelectorLabel">You play: </span>
				<span
					className={`pickWhite ${playerColor === 'white' ? 'selected' : ''}`}
					onClick={() => {
						setPlayerColor('white');
					}}
				>
					TANGERINE
				</span>
				<span
					className={`pickBlack ${playerColor === 'white' ? '' : 'selected'}`}
					onClick={() => {
						setPlayerColor('black');
					}}
				>
					LAVENDER
				</span>
			</div>
			<div className="controlSelectors">
				<span
					className={selectingTimeLimits ? 'selected' : ''}
					onClick={() => {
						setSelectingTimeLimits(true);
					}}
				>
					Time control
				</span>
				<span
					className={selectingTimeLimits ? '' : 'selected'}
					onClick={() => {
						setSelectingTimeLimits(false);
					}}
				>
					Depth control
				</span>
			</div>
			<form
				className={`timeLimit ${selectingTimeLimits ? '' : 'hidden'}`}
				onSubmit={handleSubmitTimeForm((timeForm) => {
					setGameControl({ kind: 'time', ...timeForm });
				})}
			>
				<div className="timeLimitRow">
					<span className="colorLabel">Tangerine:</span>
					<input
						className={`controlInput ${timeFormErrors.whiteLimit ? 'error' : ''}`}
						aria-invalid={timeFormErrors.whiteLimit ? 'true' : 'false'}
						{...registerTimeForm('whiteLimit', { required: true, valueAsNumber: true, min: 0 })}
					/>
					<span className="formLabel"> + </span>
					<input
						className={`controlInput ${timeFormErrors.whiteIncrement ? 'error' : ''}`}
						aria-invalid={timeFormErrors.whiteIncrement ? 'true' : 'false'}
						{...registerTimeForm('whiteIncrement', {
							required: true,
							valueAsNumber: true,
							min: 0,
						})}
					/>
				</div>
				<div className="timeLimitRow">
					<span className="colorLabel">Lavender: </span>
					<input
						className={`controlInput ${timeFormErrors.blackLimit ? 'error' : ''}`}
						aria-invalid={timeFormErrors.blackLimit ? 'true' : 'false'}
						{...registerTimeForm('blackLimit', { required: true, valueAsNumber: true, min: 0 })}
					/>
					<span className="formLabel"> + </span>
					<input
						className={`controlInput ${timeFormErrors.blackIncrement ? 'error' : ''}`}
						aria-invalid={timeFormErrors.blackIncrement ? 'true' : 'false'}
						{...registerTimeForm('blackIncrement', {
							required: true,
							valueAsNumber: true,
							min: 0,
						})}
					/>
				</div>
				<input className="playButton" type="submit" value="PLAY" />
			</form>
			<form
				className={`depthLimit ${selectingTimeLimits ? 'hidden' : ''}`}
				onSubmit={handleSubmitDepthForm((depthForm) => {
					setGameControl({ kind: 'depth', ...depthForm });
				})}
			>
				<div className="depthLimitRow">
					<span className="formLabel">Depth: </span>
					<input
						className={`controlInput ${depthFormErrors.depth ? 'error' : ''}`}
						aria-invalid={depthFormErrors.depth ? 'true' : 'false'}
						{...registerDepthForm('depth', {
							required: true,
							valueAsNumber: true,
							min: 1,
							max: 20,
						})}
					/>
				</div>
				<input className="playButton" type="submit" value="PLAY" />
			</form>
		</div>
	) : (
		<Game gameControl={gameControl} playerColor={playerColor} />
	);
}
