clk: clk.o
	g++ $(CXXFLAGS) clk.o -o clk -lncurses

clk.o:
	g++ $(CXXFLAGS) -c clk.cpp -lncurses

clean:
	rm -f clk.o clk

run: clean clk
	./clk