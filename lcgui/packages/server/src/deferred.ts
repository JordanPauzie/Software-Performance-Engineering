import assert from 'node:assert';

type Resolver<T> = (value: T | PromiseLike<T>) => void;
type Rejector = (reason: Error) => void;

/** Deferred represents a promise plus operations to resolve or reject it. */
export class Deferred<T> {
	/** The promise. */
	public readonly promise: Promise<T>;

	/** Mutator: fulfill the promise with a value of type T. */
	public readonly resolve: Resolver<T>;

	/** Mutator: reject the promise with an Error value. */
	public readonly reject: Rejector;

	/** Make a new Deferred. */
	public constructor() {
		let capturedResolve: Resolver<T> | undefined;
		let capturedReject: Rejector | undefined;

		this.promise = new Promise<T>((resolve: Resolver<T>, reject: Rejector) => {
			capturedResolve = resolve;
			capturedReject = reject;
		});

		// TypeScript's static checking doesn't know for sure
		// that the Promise constructor callback above is called synchronously,
		// so assert that resolve and reject have indeed been initialized by this point
		assert(capturedResolve);
		assert(capturedReject);
		this.resolve = capturedResolve;
		this.reject = capturedReject;
	}
}
