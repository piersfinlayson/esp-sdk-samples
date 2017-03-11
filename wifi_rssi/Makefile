# Makefile for esp-sdk-samples
SDK_BASE ?= ~/esp-open-sdk
SDK_BIN ?= $(SDK_BASE)/xtensa-lx106-elf/bin
PATH := $(SDK_BIN):$(PATH)
CC = $(SDK_BIN)/xtensa-lx106-elf-gcc
ESPTOOL ?= $(SDK_BIN)/esptool.py
CFLAGS = -Os -Iinclude -I./ -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lupgrade -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lcirom -lgcc
LDFLAGS = -Teagle.app.v6.ld

app_image-0x00000.bin: app_image
	$(ESPTOOL) elf2image $^

app_image: user_main.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

flash: app_image-0x00000.bin
	$(ESPTOOL) write_flash 0x3fc000 $(SDK_BASE)/sdk/bin/esp_init_data_default.bin
	$(ESPTOOL) write_flash 0 app_image-0x00000.bin 0x10000 app_image-0x10000.bin

connect:
	miniterm.py /dev/ttyUSB0 74880 --raw

clean: 
	@rm -f app_image* *.o
