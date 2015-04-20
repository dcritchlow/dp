all: threads
clean:
	rm threads
threads: threads.cpp Makefile
	g++ -ggdb -Wall  -pthread -D_REENTRANT -o threads threads.cpp
