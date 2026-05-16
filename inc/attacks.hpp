#pragma once

inline uint64_t bishopAttacksFrom(int from, uint64_t occupied){
	// get rank and file
	uint64_t r = from / 8;
	uint64_t f = from % 8;

	uint64_t attacks = 0;

	// all directions
	// north east
	for(int rr = r + 1, ff = f + 1; 
		rr < 8 && ff < 8;
		rr++, ff++){
	
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// north west
	for(int rr = r + 1, ff = f - 1;
		rr < 8 && ff >= 0;
		rr++, ff--){
		
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// south east
	for(int rr = r - 1, ff = f + 1;
		rr >= 0 && ff < 8;
		rr--, ff++){
		
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}

	// south west
	for(int rr = r - 1, ff = f -1;
		rr >= 0 && ff >= 0;
		rr--, ff--){
		
		int shift = rr*8 + ff;
		attacks |= 1ULL << shift;

		if (occupied & (1ULL << shift)) break;
	}
	
	return attacks;
}

inline uint64_t rookAttacksFrom(int from, uint64_t occupied){
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

