#pragma once
#include <bit>
#include <array>

class Eval{
public:
	[[no_discard]] static int evaluate(const Board& board);
private:
	static constexpr std::array<int, 6> PIECE_VALUES = {
		100,  // PAWN !ORDERING MUST BE MAINTAINED with types.hpp
		525,  // ROOK
		350,  // KNIGHT
		350,  // BISHOP
		1000, // QUEEN
		2000, // KING
	};
};
