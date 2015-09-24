all: main setup
main: main.o
	g++ -g -std=c++11 main.o include/stxxl/release/lib/libstxxl.a -lpthread -fopenmp -lpython2.7 -o search

main.o: main.cpp
	g++ -g -c -std=c++11 main.cpp -Iinclude/stxxl/include/ -Iinclude/stxxl/release/include/ -I/usr/include/python2.7

setup: setup.o
	g++ -g -std=c++11 setup.o include/stxxl/release/lib/libstxxl.a -lpthread -fopenmp -lpython2.7 -o buildindex

setup.o: setup.cpp
	g++ -g -c -std=c++11 setup.cpp -Iinclude/stxxl/include/ -Iinclude/stxxl/release/include/ -I/usr/include/python2.7

clean:
	rm setup.o main.o buildindex search
