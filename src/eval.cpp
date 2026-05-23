#include "eval.hpp"
#include "board.hpp"

int Eval::evaluate(const Board& board){
	int white{0};
	int black{0};

	for (int p = PAWN; p <= KING; ++p) {
		 
		uint64_t w = board.pieces[WHITE][p];

		while (w) {
			int sq = popLSB(w);
			white += PIECE_VALUES[p] + PST[p][sq ^ 56];
		}

		uint64_t b = board.pieces[BLACK][p]; 

		while (b) {
			int sq = popLSB(b);
			black += PIECE_VALUES[p] + PST[p][sq];
		}
	}

	int score = white - black;
	return board.turn == WHITE ? score : -score;
}

