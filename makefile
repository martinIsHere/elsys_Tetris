CC      = gcc
CFLAGS  = -Wall -Iinclude
LDFLAGS = -Llib -lraylib -lgdi32 -lwinmm

TARGET  = bin\elsys_Tetris.exe
SRC     = src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $@ $(CFLAGS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

bear:
	bear -- make clean all

clean:
	rm -f $(TARGET)

.PHONY: all run clean bear
