CXXFLAGS=-std=c++17 -Wall -Wextra -pedantic -Werror=vla -Werror=return-type -g \
-Ofast -march=native -Wno-multichar

ClockIn: ClockIn.o
	clang++ $(CXXFLAGS) ClockIn.o -o ClockIn -lncurses

ClockIn.o:
	clang++ $(CXXFLAGS) -c ClockIn.cpp

clean:
	rm -f ClockIn.o ClockIn

run: clean ClockIn
	./ClockIn