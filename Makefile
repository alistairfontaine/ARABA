# ARABA Makefile
# Zero dependencies. Pure C++17.

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I include
TARGET = araba_node
SRCS = src/araba_node.cpp src/network.cpp src/routing.cpp src/persistence.cpp src/crypto.cpp src/fragment.cpp src/config.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean run test-crypto

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) araba.log pending_messages.rue test_crypto araba.conf

run: $(TARGET)
	sudo ./$(TARGET)

test-crypto:
	$(CXX) $(CXXFLAGS) -o test_crypto src/test_crypto.cpp src/crypto.cpp
	./test_crypto
	rm -f test_crypto
