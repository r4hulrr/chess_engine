#include "move_gen.hpp"

std::vector<Move> MoveGen::generateMoves(){
	std::vector<Move> pl_moves = generatePlMoves();
	std::vector<Move> moves;

	for(const Move& m : pl_moves){
		Board next = board;
		next.makeMove(m);

		if (!next.inCheck(board.turn)) moves.push_back(m);
	}

	return moves;
}

std::vector<Move> MoveGen::generatePlMoves(){
	std::vector<Move> moves;

	generatePawnMoves(moves);
	generateRookMoves(moves);
	generateBishopMoves(moves);
	generateQueenMoves(moves);
	generateKnightMoves(moves);
	generateKingMoves(moves);

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

			MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;

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

			MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;

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

			MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;

			moves.emplace_back(makeMove(from, to, BISHOP, flag));

		}
	}
}

void MoveGen::generateQueenMoves(std::vector<Move>& moves){

	uint64_t queen = board.pieces[board.turn][QUEEN];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	while (queen){
		int from = popLSB(queen);
		
		uint64_t targets = bishopAttacksFrom(from, board.occupied) | rookAttacksFrom(from, board.occupied);
		targets &= ~own;

		while(targets){
			int to = popLSB(targets);

			MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;

			moves.emplace_back(makeMove(from, to, QUEEN, flag));

		}
	}
}

void MoveGen::generateKingMoves(std::vector<Move>& moves){
	
	uint64_t king = board.pieces[board.turn][KING];

	uint64_t own = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;

	uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

	int from = popLSB(king);
	uint64_t targets = KING_ATTACKS[from] & ~own;

	while(targets){
		int to = popLSB(targets);

		MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;

		moves.emplace_back(makeMove(from, to, KING, flag));
	}

	generateCastlingMoves(moves);
}

void MoveGen::generateCastlingMoves(std::vector<Move>& moves){
	Color attacker = board.turn;
	Color defender = opposite(attacker);

	if (attacker == WHITE){
		// white king goes from e1 to g1
		if ((board.castlingRights & WHITE_KINGSIDE) &&
		!(board.occupied & ((1ULL << F1) | (1ULL << G1))) &&
		!board.isSquareAttacked(E1, defender) &&
		!board.isSquareAttacked(F1, defender) &&
		!board.isSquareAttacked(G1, defender)) moves.emplace_back(makeMove(E1, G1, KING, KING_CASTLE));

		// white queen side e1 to c1
		if ((board.castlingRights & WHITE_QUEENSIDE) &&
		!(board.occupied & ((1ULL << D1) | (1ULL << C1) | (1ULL << B1))) &&
		!board.isSquareAttacked(E1, defender) &&
		!board.isSquareAttacked(D1, defender) &&
		!board.isSquareAttacked(C1, defender)) moves.emplace_back(makeMove(E1, C1, KING, QUEEN_CASTLE));
	}else{
		// black king goes from e8 to g8
		if ((board.castlingRights & BLACK_KINGSIDE) &&
		!(board.occupied & ((1ULL << F8) | (1ULL << G8))) &&
		!board.isSquareAttacked(E8, defender) &&
		!board.isSquareAttacked(F8, defender) &&
		!board.isSquareAttacked(G8, defender)) moves.emplace_back(makeMove(E8, G8, KING, KING_CASTLE));

		// white queen side e8 to c8
		if ((board.castlingRights & BLACK_QUEENSIDE) &&
		!(board.occupied & ((1ULL << D8) | (1ULL << C8) | (1ULL << B8))) &&
		!board.isSquareAttacked(E8, defender) &&
		!board.isSquareAttacked(D8, defender) &&
		!board.isSquareAttacked(C8, defender)) moves.emplace_back(makeMove(E8, C8, KING, QUEEN_CASTLE));
	}
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
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION));
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
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION_CAPTURE));
	}

	while (captureRightPromo) {
		int to = popLSB(captureRightPromo);
		int from = to + 7;
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION_CAPTURE));

	}

	// handle en passant
	generateBlackEnPassant(moves);
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
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION));
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
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION_CAPTURE));
	}

	while (captureRightPromo) {
		int to = popLSB(captureRightPromo);
		int from = to - 9;
		moves.emplace_back(makeMove(from, to, PAWN, QUEEN, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, ROOK, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, BISHOP, PROMOTION_CAPTURE));
		moves.emplace_back(makeMove(from, to, PAWN, KNIGHT, PROMOTION_CAPTURE));
	}

	generateWhiteEnPassant(moves);
}

void MoveGen::generateWhiteEnPassant(std::vector<Move>& moves){
	if (board.enPassantSquare == -1) return;

	uint64_t epB = 1ULL << board.enPassantSquare;
	
	// white pawn from left of captures right/up or left/up
	
	uint64_t fromLeft  = ((epB & NOT_FILE_A) >> 9) & board.pieces[WHITE][PAWN];
	uint64_t fromRight = ((epB & NOT_FILE_H) >> 7) & board.pieces[WHITE][PAWN];

	while(fromLeft){
		int from = popLSB(fromLeft);
		moves.emplace_back(makeMove(from, board.enPassantSquare, PAWN, EN_PASSANT));
	}

	while(fromRight){
		int from = popLSB(fromRight);
		moves.emplace_back(makeMove(from, board.enPassantSquare, PAWN, EN_PASSANT));
	}
}

void MoveGen::generateBlackEnPassant(std::vector<Move>& moves){
	if (board.enPassantSquare == -1) return;

	uint64_t epW = 1ULL << board.enPassantSquare;
	
	uint64_t fromLeft  = ((epW & NOT_FILE_A) << 7) & board.pieces[BLACK][PAWN];
	uint64_t fromRight = ((epW & NOT_FILE_H) << 9) & board.pieces[BLACK][PAWN];

	while(fromLeft){
		int from = popLSB(fromLeft);
		moves.emplace_back(makeMove(from, board.enPassantSquare, PAWN, EN_PASSANT));
	}

	while(fromRight){
		int from = popLSB(fromRight);
		moves.emplace_back(makeMove(from, board.enPassantSquare, PAWN, EN_PASSANT));
	}
}
