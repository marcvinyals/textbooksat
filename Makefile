CXX = g++
CPPFLAGS = -std=c++0x -Wall
LDFLAGS=
SOURCES = solver.cc cdcl.cc dimacs.cc data_structures.cc formatting.cc analysis.cc log.cc ui.cc
OBJS = $(SOURCES:.cc=.o)
ROBJS = $(addprefix release/,$(OBJS))
HEADERS = solver.h dimacs.h data_structures.h formatting.h analysis.h log.h ui.h


# argp.h under MacOSX
ifeq ($(shell uname -s),Darwin)
$(info Mac OS: using argp-standalone package)
ARGP=/opt/local/lib/libargp.a
CPPFLAGS+=-I/opt/local/include/
else
ARGP=
endif

# gcc 4.8.1 and libstdc++ path at runtime
ifeq ($(CXX),g++)
ifneq (,$(findstring gcc/4.8.1,$(LOADEDMODULES)))
$(info Using gcc/4.8.1 module)
LDFLAGS+=-L/opt/gcc/4.8.1 -Wl,-rpath=/opt/gcc/4.8.1/lib64 
endif
endif


all: sat satr

%.o : %.cc $(HEADERS)
	$(CXX) $(CPPFLAGS) -g -c -o $@ $<

sat: main.o $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -g -o  $@ $+ $(ARGP)

clean:
	rm -f sat *.o satr release/*.o
	rm -fr release/

test: test.o $(OBJS)
	$(CXX) $(CPPFLAGS) -g -o $@ $+ -lgtest -lpthread
	./test

release/%.o: %.cc $(HEADERS)
	@-mkdir -p release/
	$(CXX) $(CPPFLAGS) -O2 -DNDEBUG -c -o $@ $<

satr: release/main.o $(ROBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -O2 -o $@ $+ $(ARGP)

.PHONY : all clean test
