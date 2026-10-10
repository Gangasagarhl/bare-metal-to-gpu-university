# Oven controller build. The board file was renamed last week (board_v1.cpp -> board.cpp).
CXX      = g++
CXXFLAGS = -std=c++20 -O1 -Wall -Wextra -Werror
OBJS     = main.o sensor.o $(wildcard board_v1.o)

oven: main.o sensor.o
	$(CXX) $(CXXFLAGS) -Wl,-Map=oven.map $(OBJS) -o oven

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o oven oven.map
