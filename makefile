CFLAGS  = -V -mmcs51 --model-large --xram-size 0x1800 --xram-loc 0x0000 --code-size 0xec00 --stack-auto --Werror
CC      = sdcc
FLASHER = ../CH55x_python_flasher/chflasher.py
TARGET  = runner
OBJS	= main.rel ch559.rel serial.rel timer3.rel

all: $(TARGET).bin

program: $(TARGET).bin
	$(FLASHER) -w -f $(TARGET).bin

run: program
	$(FLASHER) -s

clean:
	rm -f *.asm *.lst *.rel *.rst *.sym $(TARGET).bin $(TARGET).ihx $(TARGET).lk $(TARGET).map $(TARGET).mem

%.rel: chlib/%.c chlib/*.h
	$(CC) -c $(CFLAGS) $<

%.rel: %.c chlib/*.h
	$(CC) -c $(CFLAGS) $<

$(TARGET).ihx: $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.bin: %.ihx
	sdobjcopy -I ihex -O binary $< $@
