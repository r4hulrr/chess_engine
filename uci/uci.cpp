#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "board.hpp"
#include "move_gen.hpp"
#include "search.hpp"

void handlePosition(Board& board, std::istringstream& args);
bool applyMove(Board& board, const std::string& token);
int parseSquare(const std::string& s);
Piece parsePromo(char c);
void handleGo(Board& board, std::istringstream& args);
std::string moveName(const Move& m);

int main(){
	std::cout << std::unitbuf;

	Board board;
	board.setStartPos();

	std::string line;
	while(std::getline(std::cin, line)){
		std::istringstream args(line);
		std::string cmd;
		args >> cmd;

		if (cmd == "uci"){
			std::cout << "id name Chess Engine\n";
			std::cout << "id author r4hulrr\n";
			std::cout << "uciok\n";
		} else if (cmd == "isready"){
			std::cout << "readyok\n";
		} else if (cmd == "ucinewgame"){
			board.setStartPos();
		} else if (cmd == "position"){
			handlePosition(board, args);
		} else if (cmd == "go"){
			handleGo(board, args);
		} else if (cmd == "quit"){
			break;
		}

		// unknown commands ignored as per spec
	}

	return 0;
}

void handleGo(Board& board, std::istringstream& args){
	std::string token;
	int depth{6};
	
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

bool applyMove(Board& board, const std::string& token){
	int from = parseSquare(token.substr(0, 2));
	int to = parseSquare(token.substr(2, 2));
	
	// if larger than normal its a promotion
	Piece promo = (token.size() >= 5) ? parsePromo(token[4]) : QUEEN; // any random piece is fine
	
	MoveGen gen(board);
	for (const Move& m : gen.generateMoves()) {
		if (m.from != from || m.to != to) continue;
        	if ((m.flags == PROMOTION || m.flags == PROMOTION_CAPTURE)
		&& m.promotionPiece != promo) continue;

		board.makeMove(m);
		return true;
	}

	return false; // illegal move
}

void handlePosition(Board& board, std::istringstream& args){
	std::string token;
	args >> token;

	if (token == "startpos"){
		board.setStartPos();
		args >> token; // consume moves
	} else if (token == "fen"){
		return;
		// will set up later
	}

	// token atp is either moves or EOL
	if (token != "moves") return;

	while(args >> token) applyMove(board, token);
}

int parseSquare(const std::string& s){ 
	return (s[1] - '1') * 8 + (s[0] - 'a');
}

Piece parsePromo(char c){
	switch (c) {
		case 'q': return QUEEN;
		case 'r': return ROOK;
		case 'b': return BISHOP;
		case 'n': return KNIGHT;
		default:  return QUEEN;
	}
}

std::string moveName(const Move& m) {

	auto convert = [](int s) -> std::string {
		return { static_cast<char>('a' + s % 8), static_cast<char>('1' + s / 8) };
	};

	std::string s = convert(m.from) + convert(m.to);

	if (m.flags == PROMOTION || m.flags == PROMOTION_CAPTURE) {
		switch (m.promotionPiece) {
			case QUEEN:  s += 'q'; break;
			case ROOK:   s += 'r'; break;
			case BISHOP: s += 'b'; break;
			case KNIGHT: s += 'n'; break;
			default: break;
		}
	}

	return s;
}
