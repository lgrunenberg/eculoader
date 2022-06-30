
CPPFLAGS= -Wpedantic -Wall -Werror -Og -std=c++11 
LDFLAGS=

ifdef OS
CC=i686-w64-mingw32-gcc
CPP=i686-w64-mingw32-c++
NAME=bin2csfile.exe
else
NAME=bin2csfile
CC=gcc
CPP=c++
endif

LD=ld

all:  $(NAME)

TOOLOBJS=\
	build/bin2csfile.o

build/bin2csfile.o:   bin2csfile.cpp
	@$(CPP) $(CPPFLAGS) $< -c  -o $@ 

$(NAME): $(TOOLOBJS)
	@$(CPP) $(LDFLAGS) -o $@ $(TOOLOBJS)

clean:
	@rm -f build/*.o
	@rm -f bin2csfile
	@rm -f bin2csfile.exe

