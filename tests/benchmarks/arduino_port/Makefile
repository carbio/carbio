# Makefile for Adafruit Arduino Library Port Benchmark

CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
LDFLAGS =
TARGET = arduino_benchmark
TEMPLATE_OPS = template_ops
SOURCES = arduino_benchmark.cpp Adafruit_Fingerprint_RPi.cpp
TEMPLATE_SOURCES = template_ops.cpp Adafruit_Fingerprint_RPi.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TEMPLATE_OBJECTS = template_ops.o Adafruit_Fingerprint_RPi.o

.PHONY: all clean

all: $(TARGET) $(TEMPLATE_OPS)

$(TARGET): arduino_benchmark.o Adafruit_Fingerprint_RPi.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEMPLATE_OPS): template_ops.o Adafruit_Fingerprint_RPi.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET) $(TEMPLATE_OPS) adafruit_arduino_results.json

run: $(TARGET)
	./$(TARGET)
