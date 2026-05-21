#include "eval.hpp"

int Eval::evaluate(const Board& board){
	int white{0};
	int black{0};
	
	for(int i = PAWN; i <= KING ; i++){
		white += std::popcount(pieces[WHITE][p]) * PIECE_VALUES[p];
		black += std::popcount(pieces[BLACK][p]) * PIECE_VALUES[p];
	}

	const int score = white - black;
	return board.turn == WHITE ? score : -score; 
}

