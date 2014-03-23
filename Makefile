CPPFLAGS = -std=c++0x -Wall -g
OBJS = main.o cdcl.o dimacs.o data_structures.o formatting.o analysis.o
LIBS = -lreadline

%.o : %.cc cdcl.h dimacs.h data_structures.h formatting.h analysis.h
	g++ $(CPPFLAGS) -c -o $@ $<

sat: $(OBJS) $(LIBS)
	g++ $(CPPFLAGS) -o $@ $+

all: sat

clean:
	rm -f sat *.o
