#include "types.hpp"

#include <bit>

// helper functions

inline int pop_lsb(uint64_t& b){
	// get idx of lsb
	auto idx = std::countr_zero(b);

	// pop the lsb
	b &= b - 1;

	return idx;
}

inline Move makeMove(int from, int to, Piece piece, MoveFlag flag){
	return Move{
		static_cast<uint8_t>(from),
		static_cast<uint8_t>(to),
		piece,
		flag
	};
}

[[nodiscard]] constexpr Color opposite(Color c){
	return c == WHITE ? BLACK ? WHITE;
}

// ranks and files
constexpr uint64_t FILE_A = 0x0101010101010101ULL;
constexpr uint64_t FILE_B = FILE_A << 1;
constexpr uint64_t FILE_C = FILE_A << 2;
constexpr uint64_t FILE_D = FILE_A << 3;
constexpr uint64_t FILE_E = FILE_A << 4;
constexpr uint64_t FILE_F = FILE_A << 5;
constexpr uint64_t FILE_G = FILE_A << 6;
constexpr uint64_t FILE_H = FILE_A << 7;

constexpr uint64_t NOT_FILE_A  = ~FILE_A;
constexpr uint64_t NOT_FILE_H  = ~FILE_H;

constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
constexpr uint64_t RANK_2 = RANK_1 << 8;
constexpr uint64_t RANK_3 = RANK_1 << 16;
constexpr uint64_t RANK_4 = RANK_1 << 24;
constexpr uint64_t RANK_5 = RANK_1 << 32;
constexpr uint64_t RANK_6 = RANK_1 << 40;
constexpr uint64_t RANK_7 = RANK_1 << 48;
constexpr uint64_t RANK_8 = RANK_1 << 56;
