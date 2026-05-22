#include "eval.hpp"
#include "board.hpp"

int Eval::evaluate(const Board& board){
	int white{0};
	int black{0};
	
	for(int i = PAWN; i <= KING ; i++){
		white += std::popcount(board.pieces[WHITE][i]) * PIECE_VALUES[i];
		black += std::popcount(board.pieces[BLACK][i]) * PIECE_VALUES[i];
	}

	const int score = white - black;
	return board.turn == WHITE ? score : -score; 
}

