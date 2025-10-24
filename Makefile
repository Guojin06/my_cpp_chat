CXX = g++
CXXFLAGS = -Wall -g -std=c++11

all: server client

server: src/server.cpp
	$(CXX) $(CXXFLAGS) src/server.cpp -o server

client: test/test_client.cpp
	$(CXX) $(CXXFLAGS) test/test_client.cpp -o client

clean:
	rm -f server client

.PHONY: all clean

