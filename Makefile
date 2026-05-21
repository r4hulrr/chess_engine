CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -Iinc -MMD -MP

SRC = \
	src/board.cpp \
	src/move_gen.cpp \
	test/main.cpp \
	test/perft.cpp

OBJ = $(SRC:%.cpp=build/%.o)
DEP = $(OBJ:.o=.d)

TARGET = build/perft

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

build/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

-include $(DEP)
