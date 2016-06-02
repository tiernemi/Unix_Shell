# compiler variables
CC = g++
CFLAGS = -Wall -O3 -std=c++11
LDLIBS = -lm -lreadline

# custom variables
objects = shell.o main.o parser.o

shell : $(objects)
	$(CC) -o $@ $(objects) $(LDLIBS) $(CFLAGS) 

main.o : main.cpp
	$(CC) -c $< $(CFLAGS) 
# test target

parser.o : parser.cpp parser.hpp
	$(CC) -c $< $(CFLAGS) 

shell.o : shell.cpp shell.hpp
	$(CC) -c $< $(CFLAGS) 

.PHONY: clean
clean:
	rm -f shell $(objects)
