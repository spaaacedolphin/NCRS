LIBS = $$(ncursesw5-config --cflags --libs) -lm

ncrs : main.o inputbox.o
	gcc -Wall -O3 -o ncrs main.o inputbox.o $(LIBS)

inputbox.o : inputbox_ncrs.c
	gcc -Wall -O3 -c -o inputbox.o inputbox_ncrs.c
main.o : main.c
	gcc -Wall -O3 -c -o main.o main.c
	
clean:
	rm *.o ncrs
