#include "search.hpp"

static int scoreMove(const Move& m, const Board& board);
static void orderMoves(std::vector<Move>& moves, const Board& board);

SearchResult Search::getBestMove(const Board& board, int depth){
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
	
	orderMoves(moves, board);

	Move bestMove;
	int bestScore{-INF};
	int alpha = -INF;
	int beta = INF;

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

	return {bestMove, bestScore, nodes};
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
	
	orderMoves(moves, board);

	int bestScore{-INF};

	for(const auto& m : moves){
		Board next = board;
		next.makeMove(m);

		int score = -negamax(next, depth - 1, -beta, -alpha);
		bestScore = std::max(bestScore, score);
		alpha = std::max(score, alpha);
		if (alpha >= beta) break; // will never be taken as opponent takes max of neg of your score
	}

	return bestScore;
}

static void orderMoves(std::vector<Move>& moves, const Board& board){
	std::sort(moves.begin(), moves.end(), [&] (const Move& a, const Move& b){
		return scoreMove(a, board) > scoreMove(b, board);
	});
}

static int scoreMove(const Move& m, const Board& board){
	// will be following 10 * victim_value - attacker_value to 
	// prefer smaller valued pieces attacking larger values
	// and adding larger values to promotion to prefer them
	if (m.flags == EN_PASSANT)
		return 10 * Eval::PIECE_VALUES[PAWN] - Eval::PIECE_VALUES[PAWN];

	if (m.flags == CAPTURE || m.flags == PROMOTION_CAPTURE){
		Piece victim = NO_PIECE;
		uint64_t to = 1ULL << m.to;
		Color defender = opposite(board.turn);
		
		// find the attacked piece
		for(int p = PAWN ; p <= KING ; p++){
			if (board.pieces[defender][p] & to){
				victim = static_cast<Piece>(p);
				break;
			}
		}

		if (victim != NO_PIECE && m.flags == CAPTURE)
			return 10*Eval::PIECE_VALUES[victim] - Eval::PIECE_VALUES[m.piece];

		if (victim != NO_PIECE && m.flags == PROMOTION_CAPTURE)
			return 10000 + 10*Eval::PIECE_VALUES[victim] - Eval::PIECE_VALUES[m.piece];

	}

	if (m.flags == PROMOTION)
		return 8000 + Eval::PIECE_VALUES[m.promotionPiece];

	return 0;
}
