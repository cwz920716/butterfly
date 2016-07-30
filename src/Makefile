CXX=clang++
CXXFLAGS=-c `llvm-config --cxxflags`
LFLAGS=-g -Wl,--export-dynamic
LLVMFLAGS=`llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native`

INCLUDES=common.h ast.h objects.h
SRCS=lexer.cpp ast.cpp codegen.cpp main.cpp objects.cpp
OBJS=lexer.o ast.o codegen.o main.o objects.o

all : $(OBJS)
	$(CXX) $(LFLAGS) $(OBJS) $(LLVMFLAGS)

lexer.o: lexer.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) lexer.cpp

ast.o: ast.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) ast.cpp

codegen.o: codegen.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) codegen.cpp

objects.o: objects.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) objects.cpp

main.o: main.cpp $(INCLUDES)
	$(CXX) $(CXXFLAGS) main.cpp

clean:
	rm -f $(OBJS) a.out
