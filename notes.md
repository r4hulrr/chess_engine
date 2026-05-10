# Chess Engine

## Representation

Its a 8x8 board and we will use bitboards that is 64 bits to represent the board

We will have
- W pawns
- B pawns
- W rooks
- B rooks
- W bishops
- B bishops
- W knights
- B knights
- W Queen
- B Queen
- W King 
- B King

Each of those will be a uint64_t that will represent its position on the board

## Move representation

Before we generate moves, we need some way of representing them. We will have a simple move block that stores:
- From
- To
- Piece
- Flags (e.g. is it a pawn promotion)

## Basic Movement Generation 

We need some way to decide what moves are possible for each piece. At this stage we are not checking legality.
E.g. If this move results in player's king being checked

### Pawns

Can move two steps forward in beginning, one step forward after, one step forward diagonal if opposing piece exists there


