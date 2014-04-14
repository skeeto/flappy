CXX      = clang++
CC       = clang
CXXFLAGS = -std=c++11 -Wall -O2
CFLAGS   = -O3 -DSQLITE_THREADSAFE=0
LDLIBS   = -lncurses -ldl -lstdc++ -lm

all : flappy

flappy : flappy.o highscores.o sqlite3.o

.PHONY : all run clean

run : flappy
	./$^

clean :
	$(RM) flappy *.o
