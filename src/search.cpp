#include "search.hpp"

SearchResult Search::getBestMove(const Board& board, int depth){
	nodes = 0;

	MoveGen gen(board);

	std::vector<Move> moves = gen.generateMoves();
	
	// gg game over
	if (moves.empty()) {
		int score;

		if (board.inCheck(board.turn)) score = -MATE_VAL;
		else score = 0;

		return SearchResult{ Move{}, score, depth, nodes };
	}

	Move bestMove;
	int bestScore{INT_MIN};
	int alpha = INT_MIN;
	int beta = INT_MAX;

	for(const auto& m : moves){
		Board next = board;
		next.makeMove(m);
		
		// opposite side makes this move so their best move is our worst
		int score = -negamax(next, depth - 1, -beta, -alpha);

		if (score > bestScore){
			bestMove = m;
			bestScore = score;
		}
		alpha = std::max(alpha, score);
	}

	return {bestMove, bestScore, depth, nodes};
}


int Search::negamax(const Board& board, int depth, int alpha, int beta){
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
	
	int bestScore{INT_MIN};

	for(const& auto m : moves){
		Board next = board;
		next.makeMove(m);

		int score = -negamax(next, depth - 1, -beta, -alpha);
		bestScore = std::max(bestScore, score);
		alpha = std::max(score, alpha);
		if (alpha >= beta) break; // will never be taken as opponent takes max of neg of your score
	}

	return bestScore;
}

