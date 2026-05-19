#pragma once
#include "types.hpp"

class Board{
public:
	uint64_t pieces[2][6];
	Color turn;

	uint64_t whiteOccupancy;
	uint64_t blackOccupancy;
	uint64_t occupied;

	[[nodiscard]] bool inCheck(Color side) const;

	[[nodiscard]] bool isSquareAttacked(int sq, Color attacker) const;

	void makeMove(const Move& move);
};

