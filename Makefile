CC = gcc
CFLAGS = -Wall -O3
LIBS = $$(ncursesw5-config --cflags --libs) -lm
objects = main.o inputbox.o celestial.o

all: ncrs

%.o : %.c
	$(CC) $(CFLAGS) -c $^

ncrs : $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) $(LIBS)

clean:
	rm *.o ncrs
