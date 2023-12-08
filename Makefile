#CFLAGS= -g -O0 -D _DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -std=c11 -fPIE -pedantic -Wall # -Werror
CFLAGS= -std=c17 -O3 -march=znver4 -D _DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -std=c11 -fPIE -pedantic -Wall -flto=full # -Werror
LDFLAGS=-lpthread -levdev -ludev -lconfig -lrt -lm -flto=full
CC=clang
OBJECTS=main.o dev_in.o dev_out.o dev_iio.o dev_evdev.o platform.o settings.o virt_ds4.o virt_ds5.o virt_mouse_kbd.o virt_evdev.o devices_status.o xbox360.o rog_ally.o
TARGET=rogue-enemy

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

include depends

depends:
	$(CC) -MM $(OBJECTS:.o=.c) > depends

clean:
	rm -f ./$(TARGET) *.o depends