# Makefile for older versions (<1.0) of Espressif "AT" firmware
CC = xtensa-lx106-elf-gcc
CFLAGS = -Os -Iinclude -I./ -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lupgrade -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lcirom -lgcc
LDFLAGS = -Teagle.app.v6.ld

app_image-0x00000.bin: app_image
	esptool.py elf2image $^

app_image: user_main.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

flash: app_image-0x00000.bin
	esptool.py write_flash 0 app_image-0x00000.bin 0x40000 app_image-0x40000.bin

connect:
	picocom -b 115200 --omap crcrlf /dev/ttyUSB0

clean: 
	@rm -f app_image* *.o
