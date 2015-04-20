all: threads dp
clean:
	rm threads
	rm dp
threads: threads.cpp Makefile
	g++ -ggdb -Wall  -pthread -D_REENTRANT -o threads threads.cpp
dp: dp.cpp Makefile
	g++ -ggdb -Wall  -pthread -D_REENTRANT -o dp dp.cpp
