#pragma once

#include <cstdint>

enum Piece : uint8_t {
	PAWN,
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING
};

enum Color : uint8_t {
	WHITE,
	BLACK
};

enum MoveFlag : uint8_t {
	QUIET,
	CAPTURE,
	DOUBLE_PAWN_PUSH,
	KING_CASTLE,
	ENPASSANT,
	PROMOTION
};

struct Board{
	uint64_t pieces[2][6];
	Color turn;

	uint64_t whiteOccupancy;
	uint64_t blackOccupancy;
	uint64_t occupied;
};

struct Move{
	uint8_t from;
	uint8_t to;
	Piece piece;
	MoveFlag flags;
};
