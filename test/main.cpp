#include <iostream>
#include "board.hpp"

uint64_t perft(Board board, int depth);
void divide(Board board, int depth);

int main() {
	Board board;
	board.setStartPos(); // use whatever function you have for initial board setup

	std::cout << "Perft 1: " << perft(board, 1) << '\n';
	std::cout << "Perft 2: " << perft(board, 2) << '\n';
	std::cout << "Perft 3: " << perft(board, 3) << '\n';
	std::cout << "Perft 4: " << perft(board, 4) << '\n';
	std::cout << "Perft 5: " << perft(board, 5) << '\n';
	std::cout << "Perft 6: " << perft(board, 6) << '\n';
	std::cout << "Perft 7: " << perft(board, 7) << '\n';
	std::cout << "Perft 8: " << perft(board, 8) << '\n';

}
