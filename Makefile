CPPFLAGS = -std=c++0x -Wall -g
OBJS = cdcl.o dimacs.o data_structures.o formatting.o analysis.o
HEADERS = cdcl.h dimacs.h data_structures.h formatting.h analysis.h

all: sat test
	./test

%.o : %.cc $(HEADERS)
	g++ $(CPPFLAGS) -c -o $@ $<

sat: main.o $(OBJS)
	g++ $(CPPFLAGS) -o $@ $+

clean:
	rm -f sat *.o

test: test.o $(OBJS)
	g++ $(CPPFLAGS) -o $@ $+ -lgtest -lpthread
