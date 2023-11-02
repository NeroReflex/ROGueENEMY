CFLAGS=-g -O0 -std=c11 -fPIE -pedantic -Wall # -Werror
LDFLAGS=-lpthread
CC=gcc
OBJECTS=main.o input_dev.o output_dev.o queue.o
TARGET=rogue_enemy

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

include depends

depends:
	$(CC) -MM $(OBJECTS:.o=.c) > depends

clean:
	rm -f ./$(TARGET) *.o depends
