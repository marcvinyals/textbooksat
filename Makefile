CPPFLAGS = -std=c++0x -Wall -g
SOURCES = cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc
OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix release/,$(OBJS))
HEADERS = cdcl.h dimacs.h data_structures.h formatting.h analysis.h log.h

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

release/%.o: %.cc $(HEADERS)
	g++ -std=c++0x -O2 -DNDEBUG -c -o $@ $<

satr: release/main.o $(ROBJS)
	g++ -std=c++0x -O2 -o $@ $+
