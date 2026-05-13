#pragma once
#include "types.hpp"
#include <vector>

class MoveGen{
public:
	moveGen(const Board& board) 
	: board(board){};

	std::vector<Move> generateMoves();
private:
	const Board& board;

	void generatePawnMoves(std::vector<Move>& moves);
	void generateRookMoves(std::vector<Move>& moves);
	void generateKnightMoves(std::vector<Move>& moves);
	void generateBishopMoves(std::vector<Move>& moves);
	void generateQueenMoves(std::vector<Move>& moves);
	void generateKingMoves(std::vector<Move>& moves);

	void generateWhitePawnMoves(std::vector<Move>& moves);
	void generateBlackPawnMoves(std::vector<Move>& moves);

	uint64_t rookAttacksFrom(int from, uint64_t occupied);
	uint64_t bishopAttacksFrom(int from, uint64_t occupied);
};
