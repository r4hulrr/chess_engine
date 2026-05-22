#pragma once
#include "eval.hpp"
#include <climits>
#include "move_gen.hpp"

static constexpr int MATE_VAL = 900000; // arbitary small value for mate so it isnt preferred

class Search{
public:
	SearchResult getBestMove(const Board& board, int depth);
private:
	long long nodes{0};

	int negamax(const Board& board, int depth, int alpha, int beta);
};
