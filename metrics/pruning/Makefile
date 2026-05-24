CXX := g++

CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -pedantic \
            -Iinc -I../../inc

BUILD := build

SRC := main.cpp \
       src/search.cpp \
       src/search_no_prune.cpp \
       ../../src/board.cpp \
       ../../src/move_gen.cpp \
       ../../src/eval.cpp

TARGET := $(BUILD)/pruning_bench

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
