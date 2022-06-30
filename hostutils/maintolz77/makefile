
CPPFLAGS= -Wpedantic -Wall -Werror -Og -std=c++11 
LDFLAGS=

ifdef OS
CC=i686-w64-mingw32-gcc
CPP=i686-w64-mingw32-c++
NAME=maintolz77.exe
else
NAME=maintolz77
CC=gcc
CPP=c++
endif

LD=ld

all:  $(NAME)

TOOLOBJS=\
	build/maintolz77.o

build/maintolz77.o:   maintolz77.cpp
	@$(CPP) $(CPPFLAGS) $< -c  -o $@ 

$(NAME): $(TOOLOBJS)
	@$(CPP) $(LDFLAGS) -o $@ $(TOOLOBJS)

clean:
	@rm -f build/*.o
	@rm -f maintolz77
	@rm -f maintolz77.exe
