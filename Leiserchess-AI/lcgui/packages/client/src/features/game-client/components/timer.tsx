type Properties = {
	readonly timeLeft: number;
	readonly isActive: boolean;
};
export default function Timer({ timeLeft, isActive: active }: Properties) {
	const seconds = Math.floor(timeLeft % 60);
	const minutes = Math.floor(timeLeft / 60);
	const minutesOutOfHour = minutes % 60;
	const hours = Math.floor(minutes / 60);
	return (
		<div
			className={`timer ${active ? 'active' : ''}`}
		>{`${hours === 0 ? '' : `${hours}:`}${minutesOutOfHour < 10 && hours > 0 ? '0' : ''}${minutesOutOfHour}:${seconds < 10 ? '0' : ''}${seconds}`}</div>
	);
}
