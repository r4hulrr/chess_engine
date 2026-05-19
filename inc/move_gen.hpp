#pragma once
#include "types.hpp"
#include <vector>

class MoveGen{
public:
	MoveGen(const Board& board) 
	: board(board){};
	
	std::vector<Move> generateMoves();
private:
	const Board& board;

	std::vector<Move> generatePlMoves();

	void generatePawnMoves(std::vector<Move>& moves);
	void generateRookMoves(std::vector<Move>& moves);
	void generateKnightMoves(std::vector<Move>& moves);
	void generateBishopMoves(std::vector<Move>& moves);
	void generateQueenMoves(std::vector<Move>& moves);
	void generateKingMoves(std::vector<Move>& moves);

	void generateCastlingMoves(std::vector<Move>& moves);
	void generateWhitePawnMoves(std::vector<Move>& moves);
	void generateBlackPawnMoves(std::vector<Move>& moves);
};
