#include "move_gen.hpp"

std::vector<Move> MoveGen::generateMoves(){
	std::vector<Move> moves;

	generatePawnMoves(moves);
	generateRookMoves(moves);
	generateBishopMoves(moves);
	generateQueenMoves(moves);

	return moves;
};

void MoveGen::generatePawnMoves(std::vector<Move>& moves){
	if (board.turn == WHITE) generateWhitePawnMoves(moves);
	else generateBlackPawnMoves(moves);
}

void MoveGen::generateRookMoves(std::vector<Move>& moves){
	uint64_t rooks = board.pieces[board.turn][ROOK];
	
	// get current turn occupied
	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	while(rooks){
		int from = popLSB(rooks);

		uint64_t targets = rookAttacksFrom(from, board.occupied);
		// remove targets that attack own pieces
		targets &= ~own;

		while (targets){
			int to = popLSB(targets);

			MoveFlag = ((1ULL << to) && enemy) ? CAPTURE : QUIET;

			moves.emplace_back(makeMove(from, to, ROOK, flag));
		}
	}
}

void MoveGen::generateKnightMoves(std::vector<Move>& moves){
	uint64_t knights = board.pieces[board.turn][KNIGHT];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	while(knights){
		int from = popLSB(knights);

		uint64_t targets = KNIGHT_ATTACKS[from] & ~own;

		while(targets){
			int to = popLSB(targets);

			MoveFlag flag = ((1ULL << to) && enemy) ? CAPTURE : QUIET;

			moves.emplace_back(makeMove(from, to, KNIGHT, flag));
		}
	}
}

void MoveGen::generateBishopMoves(std::vector<Move>& moves){
	uint64_t bishops = board.pieces[board.turn][BISHOP];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	while(bishops){
		int from = popLSB(bishops);
		
		uint64_t targets = bishopAttacksFrom(from, board.occupied);
		targets &= ~own;

		while(targets){
			int to = popLSB(targets);

			MoveFlag flag = ((1ULL << to) && enemy) ? CAPTURE : QUIET;

			moves.emplace_back(makeMove(from, to, BISHOP, flag));

		}
	}
}

void MoveGen::generateQueenMoves(std::vector<Move>& moves){

	uint64_t queen = board.pieces[board.turn][QUEEN];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	int from = popLSB(queen);
	
	uint64_t targets = bishopAttacksFrom(from, board.occupied) | rookAttacksFrom(from, board.occupied);
	targets &= ~own;

	while(targets){
		int to = popLSB(targets);

		MoveFlag flag = ((1ULL << to) && enemy) ? CAPTURE : QUIET;

		moves.emplace_back(makeMove(from, to, QUEEN, flag));

	}
}

void MoveGen::generateKingMoves(std::vector<Move>& moves){
	
	uint64_t king = board.pieces[board.turn][KING];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	uint64_t targets = KING_ATTACKS[king] & ~own;

	while(targets){
		int to = popLSB(target);

		MoveFlag flag = ((1ULL << to) && empty) ? CAPTURE : EMPTY;

		moves.emplace_back(makeMove(king, to, KING, flag));
	}
}

uint64_t MoveGen::bishopAttacksFrom(int from, uint64_t occupied){
	
	// get rank and file
	uint64_t r = from / 8;
	uint64_t f = from % 8;

	uint64_t attacks = 0;

	// all directions
	// north east
	for(int rr = r + 1, int ff = ff + 1; 
		rr < 8 && ff < 8;
		rr++, ff++){
	
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// north west
	for(int rr = r + 1, int ff = ff - 1;
		rr < 8 && ff >= 0;
		rr++, ff--){
		
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// south east
	for(int rr = r - 1, int ff = ff + 1;
		rr >= 0 && ff < 8;
		rr--, ff++){
		
		int shift = rr*8 + ff;
		attacks = 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// south west
	for(int rr = r - 1, int ff = ff -1;
		rr >= 0 && ff >= 0;
		rr--, ff--){
		
		int shift = rr*8 + ff;
		attacks = 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}
	
	return attacks;
}

void MoveGen::generateBlackPawnMoves(std::vector<Move>& moves){ 

	uint64_t pawns = board.pieces[board.turn][PAWN]; 
	uint64_t empty = ~board.occupied;

	// one move - can move only where not empty 
	uint64_t oneStep = (pawns >> 8) & empty;

	// one move can result in promotion if in final row
	uint64_t quiet = oneStep & ~RANK_1;
	uint64_t promotion = oneStep & RANK_1;

	// two moves
	uint64_t twoStep = ((oneStep & RANK_6) >> 8) & empty;
	
	while (quiet){
		int to = popLSB(quiet);
		int from = to + 8;
		moves.emplace_back(makeMove(from, to, PAWN, QUIET));
	}

	while(promotion){
		int to = popLSB(promotion);
		int from = to + 8;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

	while(twoStep){
		int to = popLSB(twoStep);
		int from = to + 16;
		moves.emplace_back(makeMove(from, to, PAWN, DOUBLE_PAWN_PUSH));
	}

	// now captures
	uint64_t captureLeft = ((pawns & NOT_FILE_A) >> 9) & board.whiteOccupancy;
	uint64_t captureRight = ((pawns & NOT_FILE_H) >> 7) & board.whiteOccupancy;

	// these can be promotions as well
	uint64_t captureLeftPromo = captureLeft & RANK_1;
	uint64_t captureLeftNormal = captureLeft & ~RANK_1;

	uint64_t captureRightPromo = captureRight & RANK_1;
	uint64_t captureRightNormal = captureRight & ~RANK_1;

	while (captureLeftNormal) {
		int to = popLSB(captureLeftNormal); 
		int from = to + 9;
		moves.emplace_back(makeMove(from, to, PAWN, CAPTURE));
	}

	while (captureRightNormal) {
		int to = popLSB(captureRightNormal);
		int from = to + 7;
		moves.emplace_back(makeMove(from, to, PAWN, CAPTURE));
	}

	while (captureLeftPromo) {
		int to = popLSB(captureLeftPromo);
		int from = to + 9;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

	while (captureRightPromo) {
		int to = popLSB(captureRightPromo);
		int from = to + 7;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

}

void MoveGen::generateWhitePawnMoves(std::vector<Move>& moves){ 

	uint64_t pawns = board.pieces[board.turn][PAWN]; 
	uint64_t empty = ~board.occupied;

	// one move - can move only where not empty 
	uint64_t oneStep = (pawns << 8) & empty;

	// one move can result in promotion if in final row
	uint64_t quiet = oneStep & ~RANK_8;
	uint64_t promotion = oneStep & RANK_8;

	// two moves
	uint64_t twoStep = ((oneStep & RANK_3) << 8) & empty;
	
	while (quiet){
		int to = popLSB(quiet);
		int from = to - 8;
		moves.emplace_back(makeMove(from, to, PAWN, QUIET));
	}

	while(promotion){
		int to = popLSB(promotion);
		int from = to - 8;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

	while(twoStep){
		int to = popLSB(twoStep);
		int from = to - 16;
		moves.emplace_back(makeMove(from, to, PAWN, DOUBLE_PAWN_PUSH));
	}

	// now captures
	uint64_t captureLeft = ((pawns & NOT_FILE_A) << 7) & board.blackOccupancy;
	uint64_t captureRight = ((pawns & NOT_FILE_H) << 9) & board.blackOccupancy;

	// these can be promotions as well
	uint64_t captureLeftPromo = captureLeft & RANK_8;
	uint64_t captureLeftNormal = captureLeft & ~RANK_8;

	uint64_t captureRightPromo = captureRight & RANK_8;
	uint64_t captureRightNormal = captureRight & ~RANK_8;

	while (captureLeftNormal) {
		int to = popLSB(captureLeftNormal); 
		int from = to - 7;
		moves.emplace_back(makeMove(from, to, PAWN, CAPTURE));
	}

	while (captureRightNormal) {
		int to = popLSB(captureRightNormal);
		int from = to - 9;
		moves.emplace_back(makeMove(from, to, PAWN, CAPTURE));
	}

	while (captureLeftPromo) {
		int to = popLSB(captureLeftPromo);
		int from = to - 7;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}

	while (captureRightPromo) {
		int to = popLSB(captureRightPromo);
		int from = to - 9;
		moves.emplace_back(makeMove(from, to, PAWN, PROMOTION));
	}
}

uint64_t MoveGen::rookAttacksFrom(int from, uint64_t occupied){
	// get rank and file
	int r = from / 8;
	int f = from % 8;

	uint64_t attacks = 0;
	
	// in all directions
	for(int i = r + 1; i < 8 ; i++){
		// get the move
		int shift = i*8 + f;
		attacks |= 1ULL << shift;
		if ((1ULL << shift) & occupied) break;
	}

	for(int i = r - 1; i >= 0; i--){
		int shift = i*8 + f;
		attacks |= 1ULL << shift;
		if ((1ULL << shift) & occupied) break;
	}

	for(int i = f + 1; i < 8; i++){
		int shift = r*8 + i;
		attacks |= 1ULL << shift;
		if ((1ULL << shift) & occupied) break;
	}

	for(int i = f - 1; i >= 0; i--){
		int shift = r*8 + i;
		attacks |= 1ULL << shift;
		if ((1ULL << shift) & occupied) break;
	}

	return attacks;
}

