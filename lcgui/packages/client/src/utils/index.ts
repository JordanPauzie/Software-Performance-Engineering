import { strict as assert } from 'node:assert';
import { match } from 'ts-pattern';
import {
	type Direction,
	type OrdinalDirection,
	type FileDirection,
	type RankDirection,
	isFileDirection,
} from '@/types/board';
import { type Rotation, rotations } from '@/types/move';

export function forRange<T>(length: number, mapping: (index: number) => T): T[] {
	return Array.from({ length }, (_, index) => mapping(index));
}

export type AtLeastOne<T> = [T, ...T[]];

const alphabet = 'abcdefghijklmnopqrstuvwxyz';

export function getFileString(file: number): string {
	const radix = alphabet.length;
	const result = [];
	let stringLength = 1;
	let stringCount = radix;
	while (file >= stringCount) {
		file -= stringCount;
		stringCount *= radix;
		stringLength++;
	}

	for (let stringDigit = 0; stringDigit < stringLength; stringDigit++) {
		result.push(alphabet[file % radix]!);
		file = Math.floor(file / radix);
	}

	return result.reverse().join('');
}

export function parseFileString(fileName: string): number {
	const radix = alphabet.length;
	const smallerStringsCount = (radix ** fileName.length - 1) / (radix - 1) - 1;
	let result = smallerStringsCount;
	let radixPow = 1;
	for (const digit of Array.from(fileName).reverse()) {
		const digitIndex = alphabet.indexOf(digit);
		assert(digitIndex !== -1);
		result += digitIndex * radixPow;
		radixPow *= radix;
	}

	return result;
}

function rotateFileDirectionRight(direction: FileDirection): RankDirection {
	return match<FileDirection, RankDirection>(direction)
		.with('north', () => 'east')
		.with('south', () => 'west')
		.exhaustive();
}

function rotateRankDirectionRight(direction: RankDirection): FileDirection {
	return match<RankDirection, FileDirection>(direction)
		.with('east', () => 'south')
		.with('west', () => 'north')
		.exhaustive();
}

export function rotateDirection(direction: Direction, rotation: Rotation) {
	for (const rotationToCheck of rotations) {
		if (rotation === rotationToCheck) {
			break;
		}

		direction = isFileDirection(direction)
			? rotateFileDirectionRight(direction)
			: rotateRankDirectionRight(direction);
	}

	return direction;
}

export function rotateOrdinalDirection(
	direction: OrdinalDirection,
	rotation: Rotation
): OrdinalDirection {
	for (const rotationToCheck of rotations) {
		if (rotation === rotationToCheck) {
			break;
		}

		direction = [rotateRankDirectionRight(direction[1]), rotateFileDirectionRight(direction[0])];
	}

	return direction;
}

export function computeDirectionRotation(
	startDirection: Direction,
	endDirection: Direction
): Rotation {
	for (const rotationToCheck of rotations) {
		if (startDirection === endDirection) {
			return rotationToCheck;
		}

		startDirection = isFileDirection(startDirection)
			? rotateFileDirectionRight(startDirection)
			: rotateRankDirectionRight(startDirection);
	}

	assert(false);
}

export function computeOrdinalDirectionRotation(
	startDirection: OrdinalDirection,
	endDirection: OrdinalDirection
): Rotation {
	for (const rotationToCheck of rotations) {
		if (startDirection[0] === endDirection[0] && startDirection[1] === endDirection[1]) {
			return rotationToCheck;
		}

		startDirection = [
			rotateRankDirectionRight(startDirection[1]),
			rotateFileDirectionRight(startDirection[0]),
		];
	}

	assert(false);
}
