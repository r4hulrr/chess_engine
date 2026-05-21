#pragma once

#include <cstdint>

enum Piece : uint8_t {
	PAWN = 0,
	ROOK = 1,
	KNIGHT = 2,
	BISHOP = 3,
	QUEEN = 4,
	KING = 5,
	NO_PIECE = 6
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

struct SearchResult{
	Move bestMove;
	int bestScore;
	long long nodes;
};

struct Move{
	uint8_t from;
	uint8_t to;
	Piece piece;
	MoveFlag flags;
	Piece promotionPiece = NO_PIECE;
};
