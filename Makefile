CXX      = g++
RM       = rm -f
CXXFLAGS = -std=c++11 -O2 -Wall
LDLIBS   = -lglut -lGLU -lGL

SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = attractor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean
