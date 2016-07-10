CXX=clang++
CXXFLAGS=-c -O3 `llvm-config --cxxflags`
LFLAGS=-g
LLVMFLAGS=`llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native`

INCLUDES=common.h ast.h
SRCS=lexer.cpp ast.cpp codegen.cpp main.cpp
OBJS=lexer.o ast.o codegen.o main.o

all : $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) $(LLVMFLAGS)

lexer.o: lexer.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) lexer.cpp

ast.o: ast.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) ast.cpp

codegen.o: codegen.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) codegen.cpp

main.o: main.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) main.cpp

clean:
	rm -f $(OBJS) a.out
