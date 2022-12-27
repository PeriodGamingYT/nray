make: main.c
	gcc main.c -o main `sdl2-config --cflags --libs` -lm

clean:
	clear
	rm -f main

run:
	make clean	
	make
	./main
