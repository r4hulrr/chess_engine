This is a walkthrough of building my chess engine. The engine in its current state is pretty basic but still employs valuable algorithms and communicates via UCI (Universal Chess Interface Protocol). Hooked up to a GUI, it plays recognizable chess and is already a decent opponent for casual players. 

I will break the project down into several parts, as shown below:

- Board Representation
- Move Representation
- Pseudo-Legal Move Generation
- Attack Tables
- Legal Move Generation
- Verification with perft
- Evaluation 
- Search: Negamax and Alpha-beta
- Move Ordering
- Piece-Square Tables
- The UCI protocol
# Board Representation

The first step and design choice is to choose a board representation. The two main choices are either piece lists or bitboards.

A piece list stores arrays of square indices, e.g. one array of squares where white pawns live, another for black pawns, knights etc. This is intuitive and often the first method that comes to us but it is not optimal. Iterating over all pieces and computing attacks would require costly loops and conditionals. Bitboards take a different approach - the entire board state is encoded as bits in a 64-bit integer, with one bit per square.

### Why bitboards?

A 64-bit integer has exactly 64 bits - the same as the number of squares on a chess board. If bit _n_ is set, there's a piece on square _n_. This means the entire board for a single piece type fits in a single `uint64_t`. More importantly, entire classes of operations become single CPU instructions: finding all squares a piece attacks, masking out friendly pieces, computing occupancy - all of these reduce to bitwise AND, OR, shifts, and a handful of hardware intrinsics.

Therefore, the board is represented as a 2D array `pieces[color][piece_type]`, giving twelve bitboards in total - one for each combination of color and piece type.

```cpp
class Board {
public:
    uint64_t pieces[2][6];  // [WHITE/BLACK][PAWN..KING]

    uint64_t whiteOccupancy;
    uint64_t blackOccupancy;
    uint64_t occupied;

    uint8_t  castlingRights;
    int      enPassantSquare{-1};
    Color    turn;
};
```

Three derived bitboards - `whiteOccupancy`, `blackOccupancy`, and `occupied` - are also included. They could always be recomputed on the fly from the twelve piece boards, but they're read so frequently that recomputing them on every lookup would add measurable overhead. Instead, we rebuild them once at the end of every move.

### Square numbering

Squares are numbered 0–63 with A1 at bit 0 and H8 at bit 63. Rank increments of 8 mean a one-rank north shift is a left-shift by 8 bits (`<< 8`) and a south shift is `>> 8`. File masks and rank masks are precomputed as constants, e.g.

```cpp
constexpr uint64_t FILE_A = 0x0101010101010101ULL;
constexpr uint64_t FILE_H = FILE_A << 7;

constexpr uint64_t NOT_FILE_A = ~FILE_A;
constexpr uint64_t NOT_FILE_H = ~FILE_H;

constexpr uint64_t RANK_1 = 0x00000000000000FFULL;
constexpr uint64_t RANK_8 = RANK_1 << 56;
```

Castling rights are stored compactly as a single byte, with one bit per right. All four rights fit in four bits and can be masked off individually or cleared together with a bitwise AND.

```cpp
constexpr uint8_t WHITE_KINGSIDE  = 1 << 0;
constexpr uint8_t WHITE_QUEENSIDE = 1 << 1;
constexpr uint8_t BLACK_KINGSIDE  = 1 << 2;
constexpr uint8_t BLACK_QUEENSIDE = 1 << 3;
```

### Iterating over a bitboard

The most common bitboard operation is iterating over set bits — finding every square occupied by pieces of a given type. The standard trick is to isolate the least significant bit, record its index, then clear it and repeat.

```cpp
inline int popLSB(uint64_t& b){
    auto idx = std::countr_zero(b);  // C++20 hardware intrinsic
    b &= b - 1;                     // clears the lowest set bit
    return idx;
}
```

`std::countr_zero` maps to a single hardware instruction on most modern CPUs. The `b &= b - 1` trick works because subtracting 1 from a number flips the lowest set bit to 0 and all lower bits to 1, so ANDing with the original clears exactly the lowest set bit and nothing else.

# Move Representation

A move needs to encode: 
- where a piece is coming from, 
- where it's going, 
- what kind of piece it is, 
- and what special thing (if anything) is happening. 

Promotions additionally need to record what the pawn is promoting to.

```cpp
struct Move {
    uint8_t   from;
    uint8_t   to;
    Piece     piece;
    MoveFlag  flags;
    Piece     promotionPiece = NO_PIECE;
};

enum MoveFlag : uint8_t {
    QUIET, 
    CAPTURE, 
    DOUBLE_PAWN_PUSH,
    KING_CASTLE, 
    QUEEN_CASTLE,
    EN_PASSANT, 
    PROMOTION, 
    PROMOTION_CAPTURE
};
```

The `from` and `to` fields are square indices (0–63) stored in a byte. The piece type is stored explicitly - this avoids having to scan all piece boards to figure out what moved when applying or scoring the move. The flag encodes everything else: whether it's a capture, a castle, a pawn double-push, en passant, or a promotion. Promotion and promotion-capture are kept as separate flags because they need to trigger different handling in `makeMove`.

Storing `piece` explicitly is a tradeoff: the struct is a few bytes larger, but it saves a scan through the piece boards in both `makeMove` and the move scorer. At typical move counts this is a net win and is good for clarity.

### Making moves

`makeMove` is a copy-and-modify function: the caller copies the board, calls `makeMove` on the copy, and the original is untouched. This avoids the complexity of an unmake function but costs a ~200-byte struct copy per node. For the depths this engine reaches, that copy is fast and the simplicity is worth it.

The logic follows a strict order: 
- remove the piece from its source square 
- handle captures (scanning the opponent's pieces at the destination),
- handle special flags (castling moves the rook too; en passant removes a pawn that isn't on the destination square; double pawn pushes set the en passant square for next move) 
- update castling rights if a king or rook moves
- place the piece on the destination (promoting if needed)
- then recompute all three occupancy bitboards

```cpp
void Board::makeMove(const Move& move){
    Color maker    = turn;
    Color defender = opposite(turn);
    uint64_t from = 1ULL << move.from;
    uint64_t to   = 1ULL << move.to;

    // Remove piece from source
    pieces[maker][move.piece] &= ~from;

    // Remove captured piece (scan defender's boards)
    if (move.flags == CAPTURE || move.flags == PROMOTION_CAPTURE){
        for(int p = PAWN; p <= KING; p++){
            if (pieces[defender][p] & to){
                pieces[defender][p] &= ~to;
                break;
            }
        }
    }

    // En passant: captured pawn is NOT on the destination square
    if (move.flags == EN_PASSANT){
        if (maker == WHITE) pieces[BLACK][PAWN] &= ~(1ULL << (move.to - 8));
        else               pieces[WHITE][PAWN] &= ~(1ULL << (move.to + 8));
    }

    // Place piece (or promotion piece) on destination
    if (move.flags == PROMOTION || move.flags == PROMOTION_CAPTURE)
        pieces[maker][move.promotionPiece] |= to;
    else
        pieces[maker][move.piece] |= to;

    // Recompute occupancies
    whiteOccupancy = blackOccupancy = 0;
    for (int p = PAWN; p <= KING; ++p){
        whiteOccupancy |= pieces[WHITE][p];
        blackOccupancy |= pieces[BLACK][p];
    }
    occupied = whiteOccupancy | blackOccupancy;

    turn = defender;
}
```

# Attack Tables

Before generating moves, the engine needs to know what squares each piece attacks from any given square. For non-sliding pieces - pawns, knights, and kings - these attack sets don't depend on the positions of other pieces, so they can be precomputed once at compile time and stored in lookup tables.

### Compile-time tables with constexpr

C++17's `constexpr` is powerful enough that the entire table generation runs at compile time. The function computes knight attacks from every square, and the result is a compile-time constant - zero runtime cost, no initialisation function needed.

```cpp
constexpr uint64_t knightAttacksFrom(int sq){
    uint64_t attacks = 0;
    int r = sq / 8, f = sq % 8;

    constexpr int dr[8] = {-2,-2,-1,-1, 1, 1, 2, 2};
    constexpr int df[8] = {-1, 1,-2, 2,-2, 2,-1, 1};

    for(int i = 0; i < 8; i++){
        int rr = r + dr[i], ff = f + df[i];
        if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8)
            attacks |= (1ULL << (rr*8 + ff));
    }
    return attacks;
}

inline constexpr auto KNIGHT_ATTACKS = generateKnightAttacks();
inline constexpr auto KING_ATTACKS   = generateKingAttacks();
```

The king attack table is generated the same way, using bitwise shifts with the file masks to prevent wrap-around: east and north-east shifts are masked with `NOT_FILE_H`, west and north-west shifts with `NOT_FILE_A`.

### Sliding piece attacks

Bishops, rooks, and queens are different — their attack sets depend on where other pieces are blocking them. A rook on A1 attacks all of the A-file when the board is empty, but only up to the first blocker when pieces are present. These can't be precomputed without knowing the occupancy.

The approach here is straightforward ray casting: walk outward from the piece's square in each direction, setting bits as you go, until you walk off the board or hit a blocker. The blocker's square is included in the attack set (because it can be captured) and then the ray stops.

```cpp
inline uint64_t rookAttacksFrom(int from, uint64_t occupied){
    int r = from / 8, f = from % 8;
    uint64_t attacks = 0;

    // North: increasing rank
    for(int i = r + 1; i < 8; i++){
        int shift = i*8 + f;
        attacks |= 1ULL << shift;
        if ((1ULL << shift) & occupied) break; // include blocker, then stop
    }
    // South, East, West follow the same pattern...
    return attacks;
}
```

This is called "classical" or "ray-casting" attack generation. It's not the fastest possible approach - most modern engines use magic bitboards, which encode the entire attack set for every square and every possible occupancy configuration into a lookup table indexed by a hash. But ray casting is intuitive and readable and fast enough for a first pass. Future work would be to extend the engine to use magic bitboards.

# Pseudo-Legal Move Generation

Pseudo-legal move generation produces all moves a piece could make ignoring whether they leave the king in check. Separating pseudo-legal from legal generation is the standard approach because it lets you generate moves cheaply in bulk and filter out the illegal ones in a separate, controlled step.
### Pawns

Pawns are the most complex piece to generate for because they have asymmetric movement, direction that depends on color, four different promotion pieces, en passant, and a special double push from the starting rank. The white and black cases are handled separately to keep the shift directions clear.

For white, a single push is `(pawns << 8) & empty`. The `& empty` ensures pawns can't push through other pieces. A double push is `((oneStep & RANK_3) << 8) & empty` - first check that the one-step result lands on rank 3 (meaning the pawn started on rank 2), then push again. Captures go diagonally: left capture is `(pawns & NOT_FILE_A) << 7`, right is `(pawns & NOT_FILE_H) << 9`, then intersected with the enemy occupancy.

```cpp
// White one-step push
uint64_t oneStep  = (pawns << 8) & empty;
uint64_t quiet    = oneStep & ~RANK_8;     // non-promotion pushes
uint64_t promo    = oneStep &  RANK_8;     // promotions
uint64_t twoStep  = ((oneStep & RANK_3) << 8) & empty;

// Captures
uint64_t capLeft  = ((pawns & NOT_FILE_A) << 7) & blackOccupancy;
uint64_t capRight = ((pawns & NOT_FILE_H) << 9) & blackOccupancy;
```
Iterating over the resulting bitboards and converting to moves is a standard popLSB loop. The `from` square for a push is always derived from `to` by reversing the shift - e.g. for a white single push, `from = to - 8`.

Promotions generate four moves each - one for queen, rook, bishop, and knight. All four are added because the best promotion piece is position-dependent (knight underpromotions occasionally win games that queen promotions lose due to stalemate), and the search will naturally find the best one.

### En passant

En passant requires care. The engine stores an `enPassantSquare` that is set when a double pawn push happens and cleared to `-1` at the start of every other move. To find white pawns that can capture en passant, the engine looks at the squares adjacent to the en passant target square and checks if a white pawn sits there.

```cpp
void MoveGen::generateWhiteEnPassant(std::vector<Move>& moves){
    if (board.enPassantSquare == -1) return;

    uint64_t epB = 1ULL << board.enPassantSquare;

    // Which white pawns are to the left/right of the ep square?
    uint64_t fromLeft  = ((epB & NOT_FILE_A) >> 9) & board.pieces[WHITE][PAWN];
    uint64_t fromRight = ((epB & NOT_FILE_H) >> 7) & board.pieces[WHITE][PAWN];
    // moves to enPassantSquare with EN_PASSANT flag
}
```

### Sliding pieces

Rooks, bishops, and queens use the ray-cast attack functions from `attacks.hpp` directly. The pattern is the same for all three: get the attack bitboard from the piece's square using the current occupancy, mask out squares occupied by friendly pieces (can't capture your own pieces), then loop over the remaining targets.

```cpp
void MoveGen::generateRookMoves(std::vector<Move>& moves){
    uint64_t rooks = board.pieces[board.turn][ROOK];
    uint64_t own   = (board.turn == WHITE) ? board.whiteOccupancy : board.blackOccupancy;
    uint64_t enemy = (board.turn == WHITE) ? board.blackOccupancy : board.whiteOccupancy;

    while(rooks){
        int from = popLSB(rooks);
        uint64_t targets = rookAttacksFrom(from, board.occupied) & ~own;

        while(targets){
            int to = popLSB(targets);
            MoveFlag flag = ((1ULL << to) & enemy) ? CAPTURE : QUIET;
            moves.emplace_back(makeMove(from, to, ROOK, flag));
        }
    }
}
```

The queen is just a rook and bishop combined - its targets are the union of rook and bishop attacks from the same square.

### Castling

Castling has three conditions beyond just having the right including: the squares between king and rook must be empty, and the king must not pass through or land on an attacked square (castling out of check is also illegal, though that's caught by the legal move filter). These three square checks are done with `isSquareAttacked` calls.

```cpp
// White kingside: king e1 → g1, rook h1 → f1
if ((board.castlingRights & WHITE_KINGSIDE)         &&
    !(board.occupied & ((1ULL << F1) | (1ULL << G1))) &&
    !board.isSquareAttacked(E1, BLACK)               &&
    !board.isSquareAttacked(F1, BLACK)               &&
    !board.isSquareAttacked(G1, BLACK))
    moves.emplace_back(makeMove(E1, G1, KING, KING_CASTLE));
```

The castling rights are updated in `makeMove` whenever a king or rook moves, and also whenever a rook's home square is captured - clearing the right for a rook that no longer exists.

# Legal Move Generation

A pseudo-legal move is illegal if it leaves the moving side's king in check. The cleanest way to test this is the make-and-verify approach: make the move on a copy of the board, then ask if the king is in check. If it is, the move is discarded.

```cpp
std::vector<Move> MoveGen::generateMoves(){
    std::vector<Move> pl_moves = generatePlMoves();
    std::vector<Move> moves;

    for(const Move& m : pl_moves){
        Board next = board;  // copy the board
        next.makeMove(m);

        if (!next.inCheck(board.turn))  // check the *original* side
            moves.push_back(m);
    }

    return moves;
}
```

Notice `board.turn`, not `next.turn` - by the time we're checking, `makeMove` has already flipped the turn, so we need to explicitly check the side that just moved.

`inCheck` finds the king square and calls `isSquareAttacked` from the opponent's perspective.

```cpp
bool Board::inCheck(Color side) const {
    uint64_t king   = pieces[side][KING];
    int      kingSq = std::countr_zero(king);
    return isSquareAttacked(kingSq, opposite(side));
}
```

`isSquareAttacked` checks each piece type in turn. For pawns, it reverses the perspective - instead of asking "what does a white pawn on every square attack", it asks "which squares can attack this square as if they were white pawns", and checks if there's actually a white pawn there.

```cpp
bool Board::isSquareAttacked(int sq, Color attacker) const {
    uint64_t target = 1ULL << sq;

    // Pawn attacks: look at which squares attack sq from the attacker's direction
    if (attacker == WHITE){
        uint64_t pawnAttackers = ((target & NOT_FILE_A) >> 9) |
                                 ((target & NOT_FILE_H) >> 7);
        if (pawnAttackers & pieces[WHITE][PAWN]) return true;
    }

    if (KNIGHT_ATTACKS[sq] & pieces[attacker][KNIGHT]) return true;
    if (KING_ATTACKS[sq]   & pieces[attacker][KING])   return true;

    if (rookAttacksFrom(sq, occupied)   & (pieces[attacker][ROOK]   | pieces[attacker][QUEEN])) return true;
    if (bishopAttacksFrom(sq, occupied) & (pieces[attacker][BISHOP] | pieces[attacker][QUEEN])) return true;

    return false;
}
```

The make-and-verify approach is simple and correct. Its cost is one board copy and one full attack check per pseudo-legal move. 

An empty move list from `generateMoves` means either checkmate or stalemate - distinguishable by calling `inCheck` on the result.

