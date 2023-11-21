#CFLAGS= -g -O0 -D _DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -std=c11 -fPIE -pedantic -Wall # -Werror
CFLAGS= -O3 -march=znver4 -D _DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -std=c11 -fPIE -pedantic -Wall -flto=full # -Werror
LDFLAGS=-lpthread -levdev -lrt -lm -flto=full
CC=clang
OBJECTS=main.o input_dev.o dev_iio.o output_dev.o queue.o logic.o platform.o virt_ds4.o
TARGET=rogue_enemy

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

include depends

depends:
	$(CC) -MM $(OBJECTS:.o=.c) > depends

clean:
	rm -f ./$(TARGET) *.o depends