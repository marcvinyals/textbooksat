CPPFLAGS = -std=c++0x -Wall -g

%.o : %.cc
	g++ $(CPPFLAGS) -c -o $@ $<

sat: main.o cdcl.o dimacs.o
	g++ $(CPPFLAGS) -o $@ $+

all: sat

clean:
	rm -f sat *.o
