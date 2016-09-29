# Sample makefile

# C++ compilation
CXX=g++
# C compilation
CC=gcc

# compile w/ 
#	debug flag -g
#	defined symbols -D
#	additional search directories for libraries -L (. is working dir)
CXXFLAGS=-g -DDEBUG -L.
CCFLAGS=$(CXXFLAGS)

all : showswitch showswitch_c philosophers

showswitch : showswitch_uthread.o libuthread.a 
	$(CXX) $(CXXFLAGS) -o $@ $< -luthread -lrt

# The same thing, but with a C compiler
showswitch_c : showswitch_uthread_bis.o libuthread.a 
	$(CC) $(CCFLAGS) -o $@ $< -luthread -lstdc++ -lrt

philosophers : philosophers_uthread.o libuthread.a
	$(CXX) $(CXXFLAGS) -o $@ $< -luthread -lrt

# (List of .o's needed to build libuthread.a)
# These are examples, yours may vary
libuthread.a : libuthread.a(uthread.o sem.o queue.o)

#  .o's in your library and their dependencies, e.g.
uthread.o : uthread.c uthread.h sem.c sem.h queue.c queue.h

# other .o's dependencies

clean :
	rm *.o *.a


