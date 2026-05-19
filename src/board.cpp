#include "board.hpp"

[[nodiscard]] bool Board::inCheck(Color side) const{
	const uint64_t king = pieces[side][KING];
	const int kingSq = std::countr_zero(king);
	return isSquareAttacked(kingSq, opposite(side));
}


[[nodiscard]] bool Board::isSquareAttacked(int sq, Color attacker) const {
	const uint64_t target = 1ULL << sq;

	// pawns
	if (attacker == WHITE){
		const uint64_t pawnAttackers = 
			((target & NOT_FILE_A) >> 9) |
			((target & NOT_FILE_H) >> 7);

		if (pawnAttackers & pieces[WHITE][PAWN]) return true;
	}else{
		const uint64_t pawnAttackers = 
		((target & NOT_FILE_A) << 7) |
		((target & NOT_FILE_H) << 9);

		if (pawnAttackers & pieces[BLACK][PAWN]) return true;
	}

	// Knights
	if (KNIGHT_ATTACKS[sq] & pieces[attacker][KNIGHT]) return true;

	// King
	if (KING_ATTACKS[sq] & pieces[attacker][KING]) return true;

	// Sliding pieces
	if (rookAttacksFrom(sq, occupied) &
	(pieces[attacker][ROOK] | pieces[attacker][QUEEN])) return true;

	if (bishopAttacksFrom(sq, occupied) &
	(pieces[attacker][BISHOP] | pieces[attacker][QUEEN])) return true;

	return false;
}

void Board::makeMove(Move& move){
	Color maker = turn;
	Color defender = opposite(turn);

	uint64_t from = 1ULL << move.from;
	uint64_t to = 1ULL << move.to;

	// remove piece from old position
	pieces[maker][move.piece] &= ~from;
	

	// handle flags
	if (move.flag == CAPTURE){
		for(int p = PAWN; p <= KING ; p++){
			if (pieces[defender][p] & to){
				pieces[defender][p] &= ~to;
				break;
			}
		}
	}

	// if castling rook piece should also be updated
	if (move.flag == KING_CASTLE){
		if (maker == WHITE){
			pieces[WHITE][ROOK] &= ~(1ULL << H1);
			pieces[WHITE][ROOK] &= ~(1ULL << F1);
		}else{
			pieces[BLACK][ROOK] &= ~(1ULL << H8);
			pieces[BLACK][ROOK] |= (1ULL << F8);
		}
	}

	if (move.flag == QUEEN_CASTLE){
		if (maker == WHITE){
			pieces[WHITE][ROOK] &= ~(1ULL << A1);
			pieces[WHITE][ROOK] &= ~(1ULL << D1);
		}else{
			pieces[BLACK][ROOK] &= ~(1ULL << A8);
			pieces[BLACK][ROOK] |= (1ULL << D8);
		}
	}
	
	// we need to reset en passant as its only valid for one move
	enPassantSquare = -1;

	if (move.flag == DOUBLE_PAWN_PUSH){
		if (maker == WHITE) enPassantSquare = move.from + 8;
		else enPassantSquare = move.from - 8;
	}

	if (move.flag == EN_PASSANT){
		if (maker == WHITE){
			int capturedSq = move.to - 8;
			pieces[BLACK][PAWN] &= ~(1ULL << capturedSq);
		}else{
			int capturedSq = move.to + 8;
			pieces[WHITE][PAWN] &= ~(1ULL << capturedSq);
		}
	}

	// update castling rights if change in kings or rook position
	if (move.piece == KING) {
		if (maker == WHITE) castlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
		else castlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
	}

	if (move.piece == ROOK) {
		if (move.from == H1) castlingRights &= ~WHITE_KINGSIDE;
		if (move.from == A1) castlingRights &= ~WHITE_QUEENSIDE;
		if (move.from == H8) castlingRights &= ~BLACK_KINGSIDE;
		if (move.from == A8) castlingRights &= ~BLACK_QUEENSIDE;
	}

	if (move.to == H1) castlingRights &= ~WHITE_KINGSIDE;
	if (move.to == A1) castlingRights &= ~WHITE_QUEENSIDE;
	if (move.to == H8) castlingRights &= ~BLACK_KINGSIDE;
	if (move.to == A8) castlingRights &= ~BLACK_QUEENSIDE;
}

	// place moving piece on new position
	pieces[maker][move.piece] |= to;

	// update occupancies
	whiteOccupancy = 0;
	blackOccupancy = 0;

	for (int p = PAWN; p <= KING; ++p) {
		whiteOccupancy |= pieces[WHITE][p];
		blackOccupancy |= pieces[BLACK][p];
	}

	occupied = whiteOccupancy | blackOccupancy;

	// switch side
	turn = defender;
}
