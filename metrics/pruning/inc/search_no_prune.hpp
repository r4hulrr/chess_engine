#pragma once

#include "search.hpp"

class SearchNP{
public:
	SearchResult getBestMove(const Board& board, int depth);
private:
	long long nodes{0};

	int negamax(const Board& board, int depth);
};
