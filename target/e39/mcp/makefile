GCCPATH="D:/ARM_GCC/6 2017-q2-update/bin/"
BASENAME=arm-none-eabi

CC=$(GCCPATH)$(BASENAME)-gcc
CPP=$(GCCPATH)$(BASENAME)-c++
LD=$(GCCPATH)$(BASENAME)-ld
OBJCOPY=$(GCCPATH)$(BASENAME)-objcopy
SIZE=$(GCCPATH)$(BASENAME)-size
GDB=$(GCCPATH)$(BASENAME)-gdb

TARGPROC=-mcpu=cortex-m3 -mthumb

SHAREDC=-ffunction-sections -O2 -Wall -Wpedantic -Werror -nostartfiles -g3 -gdwarf-2

# Some of the library code is using if flags
CODEFLAGS=-DSTM32F103C8 -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -D__ASSEMBLY__

HEADERPATHS=\
	-ISyssup/CMSIS/Device/ST/STM32F10x \
	-ISyssup/STM32F10x_StdPeriph_Driver/inc \
	-ISyssup/CMSIS/Include \
	-ISyssup/CMSIS \
	-ISyssup/CMSIS/Device \
	-ISyssup/STM32F10x_StdPeriph_Driver \
	-ISyssup/CMSIS/Device/ST

CFLAGS= -std=gnu99 $(TARGPROC) $(SHAREDC) $(CODEFLAGS) $(HEADERPATHS)

.PHONY: all flash

all : targetcode firmware

targetcode :
	@echo
	@make -C target/
	@echo

flash : all

SYSOBJS=\
	obj/startup_stm32f10x_md.o \
	obj/system_stm32f10x.o \
	obj/misc.o \
	obj/stm32f10x_adc.o \
	obj/stm32f10x_bkp.o \
	obj/stm32f10x_can.o \
	obj/stm32f10x_cec.o \
	obj/stm32f10x_crc.o \
	obj/stm32f10x_dac.o \
	obj/stm32f10x_dbgmcu.o \
	obj/stm32f10x_dma.o \
	obj/stm32f10x_exti.o \
	obj/stm32f10x_flash.o \
	obj/stm32f10x_fsmc.o \
	obj/stm32f10x_gpio.o \
	obj/stm32f10x_i2c.o \
	obj/stm32f10x_iwdg.o \
	obj/stm32f10x_pwr.o \
	obj/stm32f10x_rcc.o \
	obj/stm32f10x_rtc.o \
	obj/stm32f10x_sdio.o \
	obj/stm32f10x_spi.o \
	obj/stm32f10x_tim.o \
	obj/stm32f10x_usart.o \
	obj/stm32f10x_wwdg.o

ALLOBJS=\
	obj/mcp_trgbin.o \
	obj/main.o \
	obj/miscfunc.o \
	obj/mcp.o \
	obj/mcp_commands.o \
	obj/mcp_loader.o \
	obj/crc.o \
	obj/syscalls.o \
	obj/printf.o \
	$(SYSOBJS)



# Misc / main code
obj/main.o : main.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/miscfunc.o : miscfunc.c
	@$(CC) $(CFLAGS) -c $< -o $@

obj/mcp_trgbin.o : mcp_trgbin.s
	@$(CC) $(CFLAGS) -c $< -o $@
obj/mcp.o : mcp.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/mcp_commands.o : mcp_commands.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/mcp_loader.o : mcp_loader.c
	@$(CC) $(CFLAGS) -c $< -o $@

obj/crc.o : crc.c
	@$(CC) $(CFLAGS) -c $< -o $@

obj/printf.o : Syssup/stdio/printf.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/syscalls.o : Syssup/syscalls/syscalls.c
	@$(CC) $(CFLAGS) -c $< -o $@


# System objects
# remove -Werror
obj/startup_stm32f10x_md.o : Syssup/CMSIS/Device/ST/STM32F10x/startup_stm32f10x_md.c
	@$(CC) $(CFLAGS) -Wno-error -c $< -o $@
obj/system_stm32f10x.o : Syssup/CMSIS/Device/ST/STM32F10x/system_stm32f10x.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/misc.o : Syssup/STM32F10x_StdPeriph_Driver/src/misc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_adc.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_bkp.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_can.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_cec.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_cec.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_crc.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_dac.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_dbgmcu.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_dbgmcu.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_dma.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_exti.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_flash.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_fsmc.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_gpio.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_i2c.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_iwdg.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_pwr.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_rcc.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_rtc.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_sdio.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_sdio.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_spi.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_tim.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_usart.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
	@$(CC) $(CFLAGS) -c $< -o $@
obj/stm32f10x_wwdg.o : Syssup/STM32F10x_StdPeriph_Driver/src/stm32f10x_wwdg.c
	@$(CC) $(CFLAGS) -c $< -o $@



firmware : $(ALLOBJS)
	@$(CC) $(TARGPROC) $(SHAREDC) -Wl,-Map=bin/firmware.map -Wl,--gc-sections -Wl,-Tlink.ld -o bin/firmware.elf $(ALLOBJS)
	@rm -f obj/mcp_trgbin.o
	@$(OBJCOPY) -O binary bin/firmware.elf bin/firmware.bin
	@$(OBJCOPY) -O ihex bin/firmware.elf bin/firmware.hex
	@$(SIZE) -B bin/firmware.elf
	@echo

flash :
	@openocd -f flash.cfg

clean :
	@echo Cleaning..
	@rm -f obj/*.o
	@rm -f bin/*.elf
	@rm -f bin/*.bin
	@rm -f bin/*.hex
