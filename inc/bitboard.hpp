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

// castling rights
constexpr uint8_t WHITE_KINGSIDE  = 1 << 0;
constexpr uint8_t WHITE_QUEENSIDE = 1 << 1;
constexpr uint8_t BLACK_KINGSIDE  = 1 << 2;
constexpr uint8_t BLACK_QUEENSIDE = 1 << 3;

// square constants
enum Square {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8
};
