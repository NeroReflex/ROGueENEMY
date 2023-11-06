CFLAGS= -g -O0 -D _DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -std=c11 -fPIE -pedantic -Wall # -Werror
LDFLAGS=-lpthread -levdev -lrt
CC=gcc
OBJECTS=main.o input_dev.o output_dev.o queue.o
TARGET=rogue_enemy

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

include depends

depends:
	$(CC) -MM $(OBJECTS:.o=.c) > depends

clean:
	rm -f ./$(TARGET) *.o depends