clk: clk.o
	g++ $(CXXFLAGS) clk.o -o clk

clk.o:
	g++ $(CXXFLAGS) -c clk.cpp

clean:
	rm -f clk.o clk

run: clean clk
	./clk