#include "pl_move_gen.hpp"

class MoveGen{
public:
	MoveGen(const Board& board)
	: board(board){};

	std::vector<Move> generateMoves();

private:
	const Board& board;
	PseudoLegalMoveGen PseudoLegalMoveGen;
};
