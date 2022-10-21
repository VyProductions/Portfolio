CXXFLAGS=-std=c++17 -Wall -Wextra -pedantic -Werror=vla -Werror=return-type -g \
-Ofast -march=native -Wno-multichar

ClockIn: ClockIn.o
	g++ $(CXXFLAGS) ClockIn.o -o ClockIn -lncurses

ClockIn.o:
	g++ $(CXXFLAGS) -c ClockIn.cpp -lncurses

clean:
	rm -f ClockIn.o ClockIn

run: clean ClockIn
	./ClockIn