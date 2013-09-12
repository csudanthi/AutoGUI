all: autogui

autogui: main.o
	g++ -ggdb main.o -o autogui

main.o: main.cpp
	g++ -ggdb -c main.cpp

clean:
	rm *.o autogui
