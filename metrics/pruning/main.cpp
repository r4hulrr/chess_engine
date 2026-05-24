#include <iostream>
#include <chrono>
#include <iomanip>

#include "search.hpp"
#include "search_no_prune.hpp"
#include "board.hpp"

struct BenchStats {
	long long nodes{};
	double seconds{};
	double nps{};
};

BenchStats runNoPrune(const Board& board, int depth) {
	SearchNP search;

	auto start = std::chrono::high_resolution_clock::now();
	auto result = search.getBestMove(board, depth);
	auto end = std::chrono::high_resolution_clock::now();

	double seconds = std::chrono::duration<double>(end - start).count();

	return {
	result.nodes,
	seconds,
	result.nodes / seconds};
}

BenchStats runPrune(const Board& board, int depth) {
	Search search;

	auto start = std::chrono::high_resolution_clock::now();
	auto result = search.getBestMove(board, depth);
	auto end = std::chrono::high_resolution_clock::now();

	double seconds = std::chrono::duration<double>(end - start).count();

	return {
	result.nodes,
	seconds,
	result.nodes / seconds };
}

int main(){
	Board board;
	board.setStartPos();

	constexpr int depth = 7;

	auto np = runNoPrune(board, depth);
	auto ab = runPrune(board, depth);

	double reduction = 100.0 * (1.0 - static_cast<double>(ab.nodes) /
			static_cast<double>(np.nodes));

	std::cout << std::fixed << std::setprecision(2);

	std::cout << "Search Benchmark\n";
	std::cout << "Depth: " << depth << "\n\n";

	std::cout << "No pruning\n";
	std::cout << "Nodes: " << np.nodes << "\n";
	std::cout << "Time: " << np.seconds << " s\n";
	std::cout << "NPS: " << np.nps << "\n\n";

	std::cout << "Alpha-beta + move ordering\n";
	std::cout << "Nodes: " << ab.nodes << "\n";
	std::cout << "Time: " << ab.seconds << " s\n";
	std::cout << "NPS: " << ab.nps << "\n\n";

	std::cout << "Search-space reduction: " << reduction << "%\n";
}
