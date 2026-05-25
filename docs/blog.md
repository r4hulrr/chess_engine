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

