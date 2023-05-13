# Variables
CXX = g++
CXXFLAGS = -std=c++20 -Iinclude
TARGET = test

# Add header files as dependencies
HEADERS = $(wildcard *.hpp)

# Rules
all: $(TARGET)

$(TARGET): test.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
