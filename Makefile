CPPFLAGS = -std=c++0x -Wall -g

%.o : %.cc cdcl.h dimacs.h data_structures.h
	g++ $(CPPFLAGS) -c -o $@ $<

sat: main.o cdcl.o dimacs.o data_structures.o
	g++ $(CPPFLAGS) -o $@ $+

all: sat

clean:
	rm -f sat *.o
