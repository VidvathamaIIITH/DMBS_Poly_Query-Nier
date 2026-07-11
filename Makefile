# *****************************************************
# Variables to control Makefile operation

CXX = g++
CXXFLAGS = -g -I include -I include/catalog -I include/executor -I include/logger -I include/models -I include/parser -I include/storage

ifeq ($(OS),Windows_NT)
CXXFLAGS += -Duint="unsigned int"
endif

SRC := $(wildcard src/*.cpp) \
       $(wildcard src/catalog/*.cpp) \
       $(wildcard src/executor/*.cpp) \
       $(wildcard src/logger/*.cpp) \
       $(wildcard src/models/*.cpp) \
       $(wildcard src/parser/*.cpp) \
       $(wildcard src/storage/*.cpp)

OBJS = $(SRC:.cpp=.o)

# ****************************************************
# Targets needed to bring the executable up to date

all: server

server: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

clean:
ifeq ($(OS),Windows_NT)
	del /f /q $(subst /,\,$(OBJS)) server.exe log 2>nul || exit 0
else
	rm -f $(OBJS) *~
	rm -f server server.exe
	rm -f log
endif

%.o: %.cpp include/global.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
