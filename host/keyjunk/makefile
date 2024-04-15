CXX=c++

APP=keyapp

CXXFLAGS=-Wall -std=c++11
LDFLAGS=

.PHONY: all clean

all: $(APP)

OBJS=\
	out/main.o

out/main.o: main.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(APP): $(OBJS)
	@$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	@rm -f out/*.o
	@rm -f $(APP)
	@rm -f $(APP).exe
