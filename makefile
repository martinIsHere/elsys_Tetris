CC      = gcc
CFLAGS  = -Wall -Iinclude
LDFLAGS = -Llib -lraylib -lgdi32 -lwinmm -mwindows

TARGET  = bin\elsys_Tetris.exe
SRC     = src/main.c
RESOBJ  = resource.o

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) $(RESOBJ) -o $@ $(CFLAGS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

bear:
	bear -- make clean all

clean:
	rm -f $(TARGET)

.PHONY: all run clean bear
