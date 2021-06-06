CC = gcc
SRC = ./src
OBJ = ./.obj
BUILD = ./.build

GTK_cflags = `pkg-config --cflags --libs gtk+-3.0` -rdynamic

all: libed myapps

libed:
	$(CC) -c $(SRC)/arduino-serial-lib.c -o $(OBJ)/arduino-serial-lib.o
	$(CC) -c $(SRC)/helper.c -o $(OBJ)/helper.o $(GTK_cflags)

myapps:
	$(CC) ./main.c $(OBJ)/*.o  -o $(BUILD)/main $(GTK_cflags)
	
run:
	 $(BUILD)/main

clean:
	rm  $(BUILD)/* $(OBJ)/*