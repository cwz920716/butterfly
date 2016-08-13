CXX=g++
CXXFLAGS=--std=c++11 -c -Wall
LFLAGS=-g

INCLUDES=common.h ast.h ../lib
SRCS=lexer.cpp ast.cpp main.cpp transform.cpp
OBJS=lexer.o ast.o main.o transform.o

all : $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS)

lexer.o: lexer.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) lexer.cpp

ast.o: ast.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) ast.cpp

transform.o: transform.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) transform.cpp

main.o: main.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) main.cpp

clean:
	rm -f $(OBJS) a.out
