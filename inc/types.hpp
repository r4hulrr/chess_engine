#pragma once

#include <cstdint>

enum Piece : uint8_t {
	PAWN,
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING,
	NO_PIECE
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
	QUEEN_CASTLE,
	EN_PASSANT,
	PROMOTION,
	PROMOTION_CAPTURE
};

struct Move{
	uint8_t from;
	uint8_t to;
	Piece piece;
	MoveFlag flags;
	Piece promotionPiece = NO_PIECE;
};
