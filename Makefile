CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g $(shell root-config --cflags)
LDFLAGS=-g $(shell root-config --ldflags)
LDLIBS=-lglut -lGLU -lGL 

SRCS=main.cpp
OBJS=$(subst .cc,.o,$(SRCS))

all: tool

tool: $(OBJS)
	$(CXX) $(LDFLAGS) -o tool $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend

