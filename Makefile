CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2 -Iinc

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/chess_engine

SRCS := \
	src/board.cpp \
	src/eval.cpp \
	src/move_gen.cpp \
	src/search.cpp \
	uci/uci.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
