#include <iostream>
#include <cstdint>

#include "board.hpp"
#include "move_gen.hpp"

uint64_t perft(Board board, int depth){
	if (depth == 0) return 1;

	MoveGen gen(board);
	std::vector<Move> moves = gen.generateMoves();

	uint64_t nodes = 0;

	for(const Move& move : moves){
		Board next = board;
		next.makeMove(move);
		nodes += perft(next, depth - 1);
	}

	return nodes;
}
