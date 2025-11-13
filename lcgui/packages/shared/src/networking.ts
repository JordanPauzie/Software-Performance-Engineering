import { z } from 'zod';

const gameResult = z.union([z.literal('bwin'), z.literal('wwin'), z.literal('draw')]);
export type GameResult = z.infer<typeof gameResult>;
export const moveInfo = z.object({
	move: z.string(),
	endState: z.string(),
	keyframe: z.string(),
	laser: z.object({
		start: z.string(),
		steps: z.string().array(),
	}),
	status: z.union([gameResult, z.literal('ongoing')]),
});
export type GameStatus = z.infer<typeof moveInfo>['status'];
export const gameInfo = z.object({
	gameId: z.number(),
	moveHistory: moveInfo.array(),
	fen: z.string(),
	whiteTime: z.number().optional(),
	blackTime: z.number().optional(),
	status: z.union([gameResult, z.literal('ongoing')]),
});
export type MoveInfo = z.infer<typeof moveInfo>;
export type GameInfo = z.infer<typeof gameInfo>;
export const clientToServerSchemas = {
	startGame: {
		arguments: z.object({
			control: z.discriminatedUnion('kind', [
				z.object({
					kind: z.literal('time'),
					whiteIncrement: z.number().nonnegative(),
					whiteLimit: z.number().nonnegative(),
					blackIncrement: z.number().nonnegative(),
					blackLimit: z.number().positive(),
				}),
				z.object({ kind: z.literal('depth'), depth: z.number().int().positive() }),
			]),
			userPlaysWhite: z.boolean(),
		}),
		response: gameInfo,
	},
	cancelGame: {
		arguments: z.number(),
		response: z.boolean(),
	},
	move: { arguments: z.object({ move: z.string(), gameId: z.number() }), response: gameInfo },
	validMoves: { arguments: z.string(), response: z.array(moveInfo) },
	listenGame: { arguments: z.number(), response: gameInfo },
	analyze: {
		arguments: z.object({ fen: z.string(), depth: z.number().int().positive() }),
		response: z.number(),
	},
	cancelAnalysis: {
		arguments: z.number(),
		response: z.boolean(),
	},
	getStartFen: {
		arguments: z.null(),
		response: z.string(),
	},
};
export const serverToClientSchemas = {
	moveMade: gameInfo,
	analysisReported: z.object({ pv: z.string().array(), score: z.number(), analysisId: z.number() }),
	gameEnded: z.object({ gameInfo, gameResult }),
};
export type Untrusted<T> = T | { __brand: 'UNTRUSTED' };

export type ClientToServerResponse<EventName extends keyof typeof clientToServerSchemas> = z.infer<
	(typeof clientToServerSchemas)[EventName]['response']
>;
export type ClientToServerArgument<EventName extends keyof typeof clientToServerSchemas> = z.infer<
	(typeof clientToServerSchemas)[EventName]['arguments']
>;
export type ClientToServerCallback<EventName extends keyof typeof clientToServerSchemas> = (
	argument: Untrusted<ClientToServerResponse<EventName>>
) => void;
export type ClientToServerHandler<EventName extends keyof typeof clientToServerSchemas> = (
	argument: Untrusted<ClientToServerArgument<EventName>>,
	callback: ClientToServerCallback<EventName>
) => Promise<void>;

export type ClientToServerEvents = {
	[EventName in keyof typeof clientToServerSchemas]: ClientToServerHandler<EventName>;
};
export type ServerToClientArgument<EventName extends keyof typeof serverToClientSchemas> = z.infer<
	(typeof serverToClientSchemas)[EventName]
>;
export type ServerToClientHandler<EventName extends keyof typeof serverToClientSchemas> = (
	argument: Untrusted<ServerToClientArgument<EventName>>
) => void;

export type ServerToClientEvents = {
	[EventName in keyof typeof serverToClientSchemas]: ServerToClientHandler<EventName>;
};

export function makeServerListener<EventName extends keyof typeof clientToServerSchemas>(
	eventName: EventName,
	handler: (
		argument: ClientToServerArgument<EventName>,
		callback: (argument: ClientToServerResponse<EventName>) => void
	) => Promise<void>
): ClientToServerHandler<EventName> {
	return async (argument, callback) => {
		const result = clientToServerSchemas[eventName].arguments.safeParse(argument);
		if (!result.success) return;
		return handler(result.data, callback);
	};
}

export function makeClientCallback<EventName extends keyof typeof clientToServerSchemas>(
	eventName: EventName,
	callback: (argument: ClientToServerResponse<EventName>) => void
): ClientToServerCallback<EventName> {
	return (argument) => {
		const result = clientToServerSchemas[eventName].response.safeParse(argument);
		if (!result.success) return;
		callback(result.data);
	};
}

export function makeClientListener<EventName extends keyof typeof serverToClientSchemas>(
	eventName: EventName,
	handler: (argument: ServerToClientArgument<EventName>) => void
): ServerToClientHandler<EventName> {
	return (argument) => {
		const result = serverToClientSchemas[eventName].safeParse(argument);
		if (!result.success) return;
		handler(result.data);
	};
}
