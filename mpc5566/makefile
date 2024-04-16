PROC=powerpc

ifdef OS
TYPE=eabi
else
# You need the SPE capable compiler
TYPE=linux-gnuspe
endif

AS  = $(PROC)-$(TYPE)-as
CC  = $(PROC)-$(TYPE)-gcc
CPP = $(PROC)-$(TYPE)-cpp
AR  = $(PROC)-$(TYPE)-ar
LD  = $(PROC)-$(TYPE)-ld
NM  = $(PROC)-$(TYPE)-nm
OBJDUMP = $(PROC)-$(TYPE)-objdump
OBJCOPY = $(PROC)-$(TYPE)-objcopy
SIZE    = $(PROC)-$(TYPE)-size

ifdef OS
LZTOOL=../hostutils/maintolz77.exe
CSTOOL=../hostutils/bin2csfile.exe
else
LZTOOL=../hostutils/maintolz77
CSTOOL=../hostutils/bin2csfile
endif

MODFLAG=
# MODFLAG += -DBAMMODE
# MODFLAG += -DenableDebugBOX
# MODFLAG += -Ddisableflash
MODFLAG += -DenableBroadcast

# 401 403 405 405fp 440 440fp 464 464fp 476 476fp
# 505 601 602 603 603e 604 604e 620 630 740 7400 7450 750
# 801 821 823 8540 8548 860 970 G3 G4 G5 a2 cell
# e300c2 e300c3 e500mc e500mc64 e5500 e6500 ec603e native

# Actual: e200z6

# Not really the correct model but we get the right instruction set.
# (Avoid float just to stay safe. Don't know how it'll behave)
# -mcpu=ec603e
TARGPROC = -mspe -mno-toc -mbig-endian
GLOBFLAGS= $(MODFLAG) -Dmpc5566 -DPOWERPC
ASFLAGS  = $(TARGPROC) $(GLOBFLAGS) -c 
CFLAGS   = $(TARGPROC) $(GLOBFLAGS) -Os -Wall -Werror -Wpedantic -std=gnu99 -c

.PHONY: all

all : clean \
	out/mainloader.bin \
	out/loader.bin

# # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # #
# Main loader
MAINOBJ= \
	out/mainloader.o \
	out/mainhelper.o \
	out/mainflash.o \
	out/gmlan.o \
	out/md5s.o \
	out/md5.o

mainmessage:
	@echo Building main loader..

# Linker script
out/mainloader.ld: linkerscripts/mainloader.in
	@$(CPP) -P $(GLOBFLAGS) $< $@

out/mainloader.o : mainloader/mainloader.c
	@$(CC) $(CFLAGS) $< -o $@

out/mainhelper.o : mainloader/mainhelper.S
	@$(CC) $(ASFLAGS) $< -o $@

out/mainflash.o : mainloader/mainflash.c
	@$(CC) $(CFLAGS) $< -o $@

out/gmlan.o : ../shared/gmlan.c
	@$(CC) $(CFLAGS) $< -o $@

out/md5s.o : ../shared//md5ppc.S
	@$(CC) $(ASFLAGS) $< -o $@

out/md5.o : ../shared//md5.c
	@$(CC) $(CFLAGS) $< -o $@

out/mainloader.elf : mainmessage out/mainloader.ld $(MAINOBJ)
	@$(LD) -nostdlib -Tout/mainloader.ld -o $@ $(MAINOBJ)

# This is _NOT_ vanilla lz77. Do not use other tools than this one
out/mainloader.bin : out/mainloader.elf
	@$(OBJCOPY) -O binary  $< $@
	@$(LZTOOL) out/mainloader.bin out/mainloader.lz


# # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # #
# Preloader / packaged main loader
PREOBJ= \
	out/preloader.o \
	out/prehelper.o

premessage:
	@echo Building preloader..

# Linker script
out/preloader.ld: linkerscripts/preloader.in
	@$(CPP) -P $(GLOBFLAGS) $< $@

out/startup.o : startup.S
	@$(CC) $(ASFLAGS) $< -o $@

out/preloader.o : preloader/preloader.c
	@$(CC) $(CFLAGS) $< -o $@

out/prehelper.o : preloader/prehelper.S
	@$(CC) $(ASFLAGS) $< -o $@

# Also fetching addresses of symbols in the main binary
out/loader.elf : premessage out/preloader.ld out/startup.o $(PREOBJ)
	@$(LD) -nostdlib -R out/mainloader.elf -Tout/preloader.ld -o $@ $(PREOBJ)

out/loader.bin : out/loader.elf
	@$(OBJCOPY) -O binary  $< $@
	@rm -f Bootloader_mpc5566.cs
	@$(CSTOOL) out/loader.bin Bootloader_mpc5566
#	@cp Bootloader_mpc5566.cs ../../Trionic/TrionicCANLib/Bootloader_mpc5566.cs
	@echo " "
	@echo " "
	@echo ___Main loader_________________
	@$(SIZE) -B out/mainloader.elf $(MAINOBJ)
	@echo " "
	@echo ___Pre loader__________________
	@$(SIZE) -B out/loader.elf $(PREOBJ)
	@echo " "
ifdef OS
	@./makehelper.bat
else
	@./makehelper.sh
endif

clean :
	@echo Cleaning..
	@rm -f out/*.o
	@rm -f out/*.ld
	@rm -f out/*.elf
	@rm -f out/*.bin
	@rm -f out/*.lz
#	@del out\*.o /q >nul 2>&1
#	@del out\*.ld /q >nul 2>&1
#	@del out\*.elf /q >nul 2>&1
#	@del out\*.bin /q >nul 2>&1
#	@del out\*.lz /q >nul 2>&1
