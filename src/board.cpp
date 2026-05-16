#include "board.hpp"

[[nodiscard]] bool inCheck(Color side) const{
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

