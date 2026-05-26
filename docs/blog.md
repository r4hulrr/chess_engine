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

# Verification with Perft

Perft is a well known reliable way to know if your move generator is correct. It counts the total number of leaf nodes reachable from a given position at a given depth. The counts are well-known for standard positions and have been verified by dozens of independent engines over decades.

The implementation is a simple recursive function: at depth 0, return 1. Otherwise generate all legal moves, make each on a copy, recurse, and accumulate.

```cpp
long long perft(Board& board, int depth){
    if (depth == 0) return 1;

    MoveGen gen(board);
    auto moves = gen.generateMoves();
    long long total = 0;

    for (const Move& m : moves){
        Board next = board;
        next.makeMove(m);
        total += perft(next, depth - 1);
    }
    return total;
}
```

The standard starting position results are ([taken from the chess programming wiki](https://www.chessprogramming.org/Perft_Results)):

|Depth|Nodes|
|---|---|
|1|20|
|2|400|
|3|8,902|
|4|197,281|
|5|4,865,609|
|6|119,060,324|
When a count is wrong, the standard debugging technique is "divide perft": print the count for each root move separately, compare against a known-good engine like Stockfish (which supports the `perft` command), and narrow down which move is generating the wrong number of nodes.

# Evaluation

The evaluation function assigns a numerical score to a position from the perspective of the side to move. Positive scores are good for the side to move; negative scores are bad. This is the static assessment the engine uses when it stops searching - at a leaf node, the evaluation tells the search how promising the position is.

The evaluation here is extremely simple: material plus piece-square tables. No pawn structure, no king safety, no mobility. Simple evaluations are surprisingly competitive because the search tree compensates for a lot of what the evaluation misses - a deeper search with a simple eval often beats a shallow search with a complex eval. Both the material and PST tables values are taken from [chess programming wiki - simple evaluation function](https://www.chessprogramming.org/Simplified_Evaluation_Function).

### Material values

The piece values used are in centipawns (hundredths of a pawn), a standard unit in chess programming:

|Piece|Value (cp)|
|---|---|
|Pawn|100|
|Knight|320|
|Bishop|330|
|Rook|500|
|Queen|900|
|King|20,000|
Note bishop (330) is valued slightly higher than knight (320). This reflects the general principle that bishops are slightly better than knights in open positions, though the difference is small. The king value (20,000) is chosen to be larger than any material gain the engine could calculate, ensuring the king is never traded.
### Piece-square tables

Material alone tells the engine that a queen is worth a queen regardless of where it sits. Piece-square tables add positional nuance by assigning a bonus or penalty to each piece depending on which square it occupies. The bonus is added to the material value.

Piece-square tables encode human chess knowledge directly into the evaluation in the form of square bonuses. Rather than writing explicit rules ("knights should be in the centre", "rooks belong on open files"), you express the preference as a 64-entry table of bonuses and penalties.

All six tables are defined as compile-time `constexpr` arrays in `eval.hpp`, written from black's perspective (A8 at index 0, H1 at index 63). For black pieces the table is indexed directly. For white pieces, the square is XOR'd with 56, which flips the board vertically so rank 1 maps to rank 8 and vice versa - making the same table work symmetrically for both colors.

```cpp
int Eval::evaluate(const Board& board){
    int white{0}, black{0};

    for (int p = PAWN; p <= KING; ++p){
        uint64_t w = board.pieces[WHITE][p];
        while (w){
            int sq = popLSB(w);
            white += PIECE_VALUES[p] + PST[p][sq ^ 56];  // flip to white perspective
        }
        uint64_t b = board.pieces[BLACK][p];
        while (b){
            int sq = popLSB(b);
            black += PIECE_VALUES[p] + PST[p][sq];          // direct for black
        }
    }

    int score = white - black;
    return board.turn == WHITE ? score : -score;  // relative to side to move
}
```

The final score is returned relative to the side to move, not always from white's perspective. This is the convention that negamax requires - more on that in the search section.

# Search: Negamax and Alpha-Beta

The search is the brain of the engine. Given a position, it looks ahead a fixed number of moves, evaluates the resulting positions, and works backwards to find the move that leads to the best outcome assuming both sides play optimally.

### Minimax and the negamax formulation

The fundamental algorithm is minimax: at each node, the maximising player picks the move with the highest score, and the minimising player picks the move with the lowest score. White wants to maximise; black wants to minimise.

Negamax is an elegant reformulation: instead of tracking which side is maximising and which is minimising, every node maximises. The trick is that the score is always relative to the side to move, and when you recurse you negate the child's score. If a position is +300 for the side to move, it's -300 for the opponent. Recursing with `-negamax(next, depth-1, ...)` flips the perspective automatically.

### Alpha-beta pruning

Plain negamax is exponential in the branching factor. A chess position has roughly 30 legal moves on average, so depth 6 means 30^6 = 729 million nodes. Alpha-beta pruning eliminates branches that cannot possibly affect the result.

The two parameters, `alpha` and `beta`, represent a window of scores that matter. Alpha is the best score the current player is already guaranteed to achieve from a parent node. Beta is the best score the opponent is already guaranteed - its a ceiling, because if the current position scores above beta, the opponent would never allow it to be reached in the first place.

When a move scores above beta, we immediately stop searching this node. The opponent won't choose this line because they already have something better. This is the cutoff, and in well-ordered positions it severly reduces the effective branching factor.

```cpp
int Search::negamax(const Board& board, int depth, int alpha, int beta){
    if (depth == 0) return Eval::evaluate(board);

    MoveGen gen(board);
    auto moves = gen.generateMoves();

    if (moves.empty()){
        if (board.inCheck(board.turn)) return -MATE_VAL - depth; // checkmate
        return 0; // stalemate
    }

    int bestScore = -INF;
    for(const Move& m : moves){
        Board next = board;
        next.makeMove(m);

        int score = -negamax(next, depth - 1, -beta, -alpha);
        bestScore  = std::max(bestScore, score);
        alpha      = std::max(alpha, score);
        if (alpha >= beta) break; // alpha-beta cutoff
    }
    return bestScore;
}
```

### Mate scoring

Checkmate scores use `-MATE_VAL - depth`. The subtraction of depth is important: it makes a faster checkmate score higher than a slower one. Without it, the engine would be indifferent between mating in 1 and mating in 10, and might play aimlessly once it "knows" it's winning.

The mate value (900,000) is chosen to be larger than any material score the engine could compute, ensuring checkmate always dominates material considerations.

### The root call

`getBestMove` handles the search root separately from the recursive negamax. The root needs to track not just the best score but the actual best move, so it maintains a `bestMove` variable updated whenever a child score exceeds the current best.

```cpp
SearchResult Search::getBestMove(const Board& board, int depth){
    MoveGen gen(board);
    auto moves = gen.generateMoves();

    Move bestMove;
    int  bestScore{-INF};
    int  alpha = -INF, beta = INF;

    for(const auto& m : moves){
        Board next = board;
        next.makeMove(m);

        int score = -negamax(next, depth - 1, -beta, -alpha);
        if (score > bestScore){
            bestMove  = m;
            bestScore = score;
        }
        alpha = std::max(alpha, score);
    }

    return {bestMove, bestScore, nodes};
}
```

# Move Ordering

Alpha-beta pruning is only as good as the move ordering. The best case is searching the best move first every time. In the worst case - we always search the worst moves first - pruning thereby does nothing here. Move ordering is therefore one of the highest-leverage things you can do to improve search performance.

The scoring heuristic prioritises moves in this order: promotion-captures, captures ordered by MVV-LVA, quiet promotions, and everything else scored zero. MVV-LVA (Most Valuable Victim, Least Valuable Attacker) is the key idea: prefer capturing a queen with a pawn over capturing a pawn with a queen, because the queen capture is more likely to be good.

```cpp
static int scoreMove(const Move& m, const Board& board){
	// will be following 10 * victim_value - attacker_value to 
	// prefer smaller valued pieces attacking larger values
	// and adding larger values to promotion to prefer them
	if (m.flags == EN_PASSANT)
		return 10 * Eval::PIECE_VALUES[PAWN] - Eval::PIECE_VALUES[PAWN];

	if (m.flags == CAPTURE || m.flags == PROMOTION_CAPTURE){
		Piece victim = NO_PIECE;
		uint64_t to = 1ULL << m.to;
		Color defender = opposite(board.turn);
		
		// find the attacked piece
		for(int p = PAWN ; p <= KING ; p++){
			if (board.pieces[defender][p] & to){
				victim = static_cast<Piece>(p);
				break;
			}
		}

		if (victim != NO_PIECE && m.flags == CAPTURE)
			return 10*Eval::PIECE_VALUES[victim] - Eval::PIECE_VALUES[m.piece];

		if (victim != NO_PIECE && m.flags == PROMOTION_CAPTURE)
			return 10000 + 10*Eval::PIECE_VALUES[victim] - Eval::PIECE_VALUES[m.piece];

	}

	if (m.flags == PROMOTION)
		return 8000 + Eval::PIECE_VALUES[m.promotionPiece];

	return 0;
}

```

The formula `10 * victim_value - attacker_value` ensures that among all captures of a given victim, we prefer the cheapest attacker. Multiplying the victim by 10 means a pawn capturing a queen ranks far above a queen capturing a pawn, for example. The numbers work out so any capture of a more valuable piece by a less valuable piece scores positively.

Moves are sorted with `std::sort` before the main loop in negamax for efficient and quick pruning.

# The UCI Protocol

UCI (Universal Chess Interface) is the standard protocol that lets a chess engine communicate with a GUI like Cutechess, Arena, or Lichess's bot infrastructure. The protocol is text-based over stdin/stdout: the GUI sends commands, the engine replies. The engine runs in its own process; the GUI manages the clock and display.

### The handshake

When the GUI connects, it sends `uci`. The engine replies with its name, author, and `uciok`. The GUI then sends `isready`, and the engine replies `readyok` once it's done initialising. The main loop just reads lines and dispatches based on the command token.

```cpp
while(std::getline(std::cin, line)){
    std::istringstream args(line);
    std::string cmd;
    args >> cmd;

    if      (cmd == "uci")       { std::cout << "id name Chess Engine\n"
                                             << "id author r4hulrr\n"
                                             << "uciok\n"; }
    else if (cmd == "isready")  { std::cout << "readyok\n"; }
    else if (cmd == "position") { handlePosition(board, args); }
    else if (cmd == "go")       { handleGo(board, args); }
    else if (cmd == "quit")     { break; }
}
```

The `position` command tells the engine what position to search. It comes in two forms: `position startpos moves e2e4 e7e5 ...`which sets up the starting position and then applies a sequence of moves, and `position fen <fen_string> moves ...` which sets up from a FEN string. This engine currently only handles `startpos`; FEN parsing is straightforward to add later.

Applying the move list works by generating all legal moves and finding the one that matches the `from`/`to`/`promotion` in the token.

```cpp
bool applyMove(Board& board, const std::string& token){
    int from  = parseSquare(token.substr(0, 2));
    int to    = parseSquare(token.substr(2, 2));
    Piece promo = (token.size() >= 5) ? parsePromo(token[4]) : QUEEN;

    MoveGen gen(board);
    for (const Move& m : gen.generateMoves()){
        if (m.from != from || m.to != to) continue;
        if ((m.flags == PROMOTION || m.flags == PROMOTION_CAPTURE)
            && m.promotionPiece != promo) continue;
        board.makeMove(m);
        return true;
    }
    return false;
}
```

### The go command

The `go` command tells the engine to start searching. The most common form for a real game is `go wtime 60000 btime 60000 winc 500 binc 500`, giving remaining clock times and increments in milliseconds. The engine parses these and allocates a time budget before calling the search.

The engine currently only accepts `go depth N` for a fixed-depth search, which is how it was first tested - it's the simplest form and useful for perft and capable of playing matches. The output is a required `info` line (which the GUI uses to show depth, score, and principal variation) followed by `bestmove`.

```cpp
void handleGo(Board& board, std::istringstream& args){
	std::string token;
	int depth{8};
	
	// only handling depth for now
	while(args >> token){
		if (token == "depth") args >> depth;
	}

	Search s;
	SearchResult result = s.getBestMove(board, depth);

	std::cout << "info depth " << depth
		<< " score cp "  << result.bestScore
		<< " nodes "     << result.nodes
		<< " pv "        << moveName(result.bestMove)
		<< "\n";

	std::cout << "bestmove " << moveName(result.bestMove) << "\n";
}
```

**One important detail**
The UCI spec requires `std::cout << std::unitbuf` at startup. Without it, stdout may be buffered and the GUI will hang waiting for output that has been written but not flushed. 

# What comes next

The engine as described is complete and playable. It correctly generates all legal moves, evaluates positions with material and piece-square bonuses, searches with negamax and alpha-beta pruning, and communicates via UCI. We can hook it up to a GUI like cutechess and play against it.

The natural next improvements, roughly in order of impact, are:

- quiescence search
- iterative deepening 
- adding time management to UCI (allocate a per-move budget from the remaining clock)
- a transposition table 
- and eventually magic bitboards for faster move generation at deeper depths.

Each of these is a substantial improvement on its own. The engine described here is a solid foundation for all of them - the architecture is clean enough that each feature can be added in isolation, and will be added in v2.
