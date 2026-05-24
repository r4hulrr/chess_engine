#include "search_no_prune.hpp"

SearchResult SearchNP::getBestMove(const Board& board, int depth){
	nodes = 0;

	MoveGen gen(board);

	std::vector<Move> moves = gen.generateMoves();
	
	// gg game over
	if (moves.empty()) {
		int score;

		if (board.inCheck(board.turn)) score = -MATE_VAL;
		else score = 0;

		return SearchResult{ Move{}, score, nodes };
	}
	
	Move bestMove;
	int bestScore{-INF};

	for(const auto& m : moves){
		Board next = board;
		next.makeMove(m);
		
		// opposite side makes this move so their best move is our worst
		int score = -negamax(next, depth - 1);

		if (score > bestScore){
			bestMove = m;
			bestScore = score;
		}
	}

	return {bestMove, bestScore, nodes};
}


int SearchNP::negamax(const Board& board, int depth){
	nodes++;

	if (depth == 0) return Eval::evaluate(board);
	
	MoveGen gen(board);

	std::vector<Move> moves = gen.generateMoves();
	
	// no moves and your turn
	if (moves.empty()){
		// checkmate is bad but a faster checkmate is worse
		// so we account for depth , lower depth the better
		if (board.inCheck(board.turn)) return -MATE_VAL - depth;

		// else stalemate
		return 0;
	}
	
	int bestScore{-INF};

	for(const auto& m : moves){
		Board next = board;
		next.makeMove(m);

		int score = -negamax(next, depth - 1);
		bestScore = std::max(bestScore, score);
	}

	return bestScore;
}
