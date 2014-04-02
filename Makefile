CPPFLAGS = -std=c++0x -Wall -g
SOURCES = cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc
OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix release/,$(OBJS))
HEADERS = cdcl.h dimacs.h data_structures.h formatting.h analysis.h log.h

ifeq ($(shell uname -s),Darwin)
ARGP=/opt/local/lib/libargp.a
else
ARGP=
endif



all: sat test
	./test

%.o : %.cc $(HEADERS)
	g++ $(CPPFLAGS) -c -o $@ $<

sat: main.o $(OBJS)
	g++ $(CPPFLAGS) -o $@ $+ $(ARGP)

clean:
	rm -f sat *.o satr release/*.o
	rm -fr release/

test: test.o $(OBJS)
	g++ $(CPPFLAGS) -o $@ $+ -lgtest -lpthread

release/%.o: %.cc $(HEADERS)
	@-mkdir -p release/
	g++ -std=c++0x -O2 -DNDEBUG -c -o $@ $<

satr: release/main.o $(ROBJS)
	g++ -std=c++0x -O2 -o $@ $+ $(ARGP)
