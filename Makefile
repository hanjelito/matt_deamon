CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRC = daemon.cpp
TARGET = daemon

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)