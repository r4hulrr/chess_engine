#include "move_gen.hpp"

std::vector<Move> MoveGen::generateMoves();

void MoveGen::generatePawnMoves(std::vector<Move>& moves){
	if (board.turn == WHITE) generateWhiteMoves(moves);
	else generateBlackMoves(moves);
}

void MoveGen::generateRookMoves(std::vector<Move>& moves);

void MoveGen::generateKnightMoves(std::vector<Move>& moves);

void MoveGen::generateBishopMoves(std::vector<Move>& moves);

void MoveGen::generateQueenMoves(std::vector<Move>& moves);

void MoveGen::generateKingMoves(std::vector<Move>& moves);

void MoveGen::generateBlackPawnMoves(std::vector<Move>& moves){ 

}

void MoveGen::generateWhitePawnMoves(std::vector<Move>& moves){ 

	uint64_t pawns = board.pieces[board.turn][PAWN]; 
	uint64_t empty = ~board.occupied;

	// one move - can move only where not empty 
	uint64_t oneStep = (pawns << 8) & empty;

	// one move can result in promotion if in final row
	uint64_t quiet = oneStep & ~RANK_8;
	uint64_t promotion = oneStep & RANK_8;

	// two moves
	uint64_t twoStep = ((oneStep & RANK_3) << 8) & empty;
	
	while (quiet){
		int to = popLSB(quiet);
		int from = to - 8;
		moves.emplace_back(makeMove(from, to, PAWN, QUIET));
	}

	while(promotion){
		int to = popLSB(quiet);
		int from = to - 8;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

	while(twoStep){
		int to = popLSB(promotion);
		int from to - 16;
		moves.emplace_back(makeMove(from, to, PAWN, DOUBLE_PAWN_PUSH));
	}
}

