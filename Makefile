VERSION  = 1.0.0
CXX      = clang++
CC       = clang
CXXFLAGS = -std=c++11 -Wall -O2 -DVERSION=$(VERSION)
CFLAGS   = -O3 -DSQLITE_THREADSAFE=0
LDLIBS   = -lncurses -ldl -lstdc++ -lm

all : flappy

flappy : flappy.o highscores.o sqlite3.o

.PHONY : all run clean archive

run : flappy
	./$^

clean :
	$(RM) flappy *.o *.tar.gz

archive : flappy-$(VERSION).tar.gz

flappy-$(VERSION).tar.gz : *.cc *.hh
	git archive --format tar --prefix flappy-$(VERSION)/ HEAD | gzip > $@
