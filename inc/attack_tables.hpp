#pragma once
#include <array>

constexpr bool onBoard(int r, int f){
	return r >= 0 && r < 8 && f >= 0 && f < 8;
}

constexpr uint64_t knightAttacksFrom(int sq){
	uint64_t attacks = 0;

	int r = sq / 8;
	int f = sq % 8;

	// knight has 8 possible moves
	constexpr int dr[8] = {-2,-2,-1,-1,1,1,2,2};
	constexpr int df[8] = {-1, 1,-2, 2,-2,2,-1,1};

	for(int i = 0; i < 8; i++){
		int rr = r + dr[i];
		int ff = f + df[i];

		if (onBoard(rr, ff)){
			int target = rr*8 + ff;
			attacks |= (1ull << target); 
		}
	}

	return attacks;
}

constexpr std::array<uint64_t, 64> generateKnightAttacks(){
	// generate attack tables for every possible knight position
	// possible as knight can jump so no blockers for pseudo legal
	std::array<uint64_t, 64> table{};

	for(int i = 0; i < 64 ; i++){
		table[i] = knightAttacksFrom(i);
	}

	return table;
}

constexpr uint64_t kingAttacksFrom(int sq){
	uint64_t king = 1ULL << sq;
	uint64_t attacks = 0;

	attacks |= king << 8;   // north
	attacks |= king >> 8;   // south

	attacks |= (king & NOT_FILE_H) << 1; // east
	attacks |= (king & NOT_FILE_A) >> 1; // west

	attacks |= (king & NOT_FILE_H) << 9; // north-east
	attacks |= (king & NOT_FILE_A) << 7; // north-west

	attacks |= (king & NOT_FILE_H) >> 7; // south-east
	attacks |= (king & NOT_FILE_A) >> 9; // south-west
	
	return attacks;
}

constexpr std::array<uint64_t, 64> generateKingAttacks(){
	
	std::array<uint64_t, 64> table{};

	for(int i = 0; i < 64 ; i++){
		table[i] = kingAttacksFrom(i);
	}

	return table;
}

inline constexpr auto KNIGHT_ATTACKS = generateKnightAttacks();
inline constexpr auto KING_ATTACKS = generateKingAttacks();
