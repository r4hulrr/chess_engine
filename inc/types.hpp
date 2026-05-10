#pragma once

enum class Piece{
	Pawn,
	Rook,
	Knight,
	Bishop,
	Queen,
	King
};

enum class Color{
	White,
	Black
};

struct Board{
	uint64_t pieces[2][6];
	Color turn;
};

struct Move{
	uint8_t from;
	uint8_t to;
	Piece piece;
	uint8_t flags;
};
