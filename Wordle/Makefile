CXXFLAGS=-std=c++17 -Wall -Wextra -pedantic -Werror=vla -Werror=return-type -g \
-Ofast -march=native -Wno-multichar

Wordle: Wordle.o
	clang++ $(CXXFLAGS) Wordle.o -o Wordle -lncurses

Wordle.o:
	clang++ $(CXXFLAGS) -c Wordle.cpp

clean:
	rm -f Wordle.o Wordle

run: clean Wordle
	./Wordle