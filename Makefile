CXX      = clang++
CXXFLAGS = -std=c++11 -Wall
LDLIBS   = -lncurses

flappy : flappy.cc

.PHONY : run clean

run : flappy
	./$^

clean :
	$(RM) flappy
