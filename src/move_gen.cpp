#include "move_gen.hpp"

std::vector<Move> MoveGen::generateMoves();

void MoveGen::generatePawnMoves(std::vector<Move>& moves){
	auto& ref = board.pieces[board.turn][Piece::pawn];
}

void MoveGen::generateRookMoves(std::vector<Move>& moves);

void MoveGen::generateKnightMoves(std::vector<Move>& moves);

void MoveGen::generateBishopMoves(std::vector<Move>& moves);

void MoveGen::generateQueenMoves(std::vector<Move>& moves);

void MoveGen::generateKingMoves(std::vector<Move>& moves);
