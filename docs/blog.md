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
