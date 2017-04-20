PLUGIN = mongodb.input
CXX = g++

CFLAGS=-O3 -std=c++11 -pg -g -c -Wall

CXXFLAGS = $(shell mapnik-config --cflags)  $(shell pkg-config --cflags --libs libmongocxx) -fPIC
LDFLAGS = -shared $(shell mapnik-config --libs) -lmapnik-json -lmongocxx -lboost_thread -lboost_filesystem -lboost_system

SOURCES := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SOURCES))

all: $(PLUGIN)

$(PLUGIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) -c $< $(CXXFLAGS) -o $@

clean:
	rm -f $(PLUGIN) $(OBJS)
