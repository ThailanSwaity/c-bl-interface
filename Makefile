c-bl-interface: main.c bluetooth.c
	gcc main.c bluetooth.c -o c-bl-interface -I/usr/local/include/ -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -L/usr/local/lib/ -L/usr/lib -lraylib -lm -lglib-2.0 -lgio-2.0 -lgobject-2.0
