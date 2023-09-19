
AS=sdas6808
LD=sdld6808
CC=sdcc
OBJCPY=sdobjcopy

PROCESSOR=hc08

CFLAGS=-m$(PROCESSOR)

ASFLAGS=

.PHONY: all

all : loader

LOADEROBJS=\
	obj/loader.rel
# obj/test.rel

obj/loader.rel : loader.s
	@$(AS) $(ASFLAGS) -o $@ $<

obj/test.rel : test.c
	@$(CC) -c $(CFLAGS) -o $@ $<

# --code-loc 0xb0 --data-loc 0x40
loader : $(LOADEROBJS)
	@echo - - - - - - - - - - - -
	@echo - Building main loader
	@$(LD) -n -i bin/loader.ihx $^
	@$(OBJCPY) -Iihex -O binary bin/loader.ihx bin/loader.bin
	@wc -c bin/loader.bin	

clean:
	@echo - - - - - - - - - - - -
	@echo - Cleaning
	@rm -f obj/*.o
	@rm -f obj/*.rel
	@rm -f obj/*.asm
	@rm -f obj/*.map
	@rm -f obj/*.rst
	@rm -f obj/*.lst
	@rm -f obj/*.lk
	@rm -f obj/*.sym
	@rm -f bin/*.bin
	@rm -f bin/*.ihx
