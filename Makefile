CXX = g++
CPPFLAGS = -std=c++0x -Wall
SOURCES = cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc
OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix release/,$(OBJS))
HEADERS = cdcl.h dimacs.h data_structures.h formatting.h analysis.h log.h

ifeq ($(shell uname -s),Darwin)
ARGP=/opt/local/lib/libargp.a
CPPFLAGS+=-I/opt/local/include/
else
ARGP=
endif



all: sat test
	./test

%.o : %.cc $(HEADERS)
	$(CXX) $(CPPFLAGS) -g -c -o $@ $<

sat: main.o $(OBJS)
	$(CXX) $(CPPFLAGS) -g -o $@ $+ $(ARGP)

clean:
	rm -f sat *.o satr release/*.o
	rm -fr release/

test: test.o $(OBJS)
	$(CXX) $(CPPFLAGS) -g -o $@ $+ -lgtest -lpthread

release/%.o: %.cc $(HEADERS)
	@-mkdir -p release/
	$(CXX) $(CPPFLAGS) -O2 -DNDEBUG -c -o $@ $<

satr: release/main.o $(ROBJS)
	$(CXX) $(CPPFLAGS) -O2 -o $@ $+ $(ARGP)
