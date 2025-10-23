CC = gcc
CFLAGS = -Wall -Wextra
INCLUDES = -I./src/ -I/usr/local/include/ -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
LIBS = -L/usr/local/lib/ -L/usr/lib

c-bl-interface: main.c ./src/bluetooth.c ./src/dict.c
	$(CC) $(CFLAGS) main.c ./src/bluetooth.c ./src/dict.c -o ./bin/c-bl-interface $(INCLUDES) $(LIBS) -lraylib -lm -lglib-2.0 -lgio-2.0 -lgobject-2.0

clean:
	rm -f ./bin/c-bl-interface
