CFLAGS  ?=  -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion \
            -Wformat-truncation -fno-common -Wconversion \
            -g3 -Os -ffunction-sections -fdata-sections -I. \
            -I. -Iinclude \
            -Icmsis_core/CMSIS/Core/Include \
            -Icmsis_f1/Include \
            -mcpu=cortex-m3 -mthumb $(EXTRA_CFLAGS)
LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map
SOURCES = main.c syscalls.c sysinit.c button.c adc.c sensor.c
SOURCES += cmsis_f1/Source/Templates/gcc/startup_stm32f103xb.s

ifeq ($(OS),Windows_NT)
  RM = cmd /C del /Q /F
else
  RM = rm -f
endif

build: firmware.bin

firmware.elf: $(SOURCES) hal.h link.ld Makefile
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(LDFLAGS) -o $@

firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary $< $@

flash: firmware.bin
	st-flash --reset write $< 0x8000000

cmsis_core:
	git clone -q --depth 1 -b 5.9.0 https://github.com/ARM-software/CMSIS_5 $@

cmsis_f1:
	git clone -q --depth 1 https://github.com/STMicroelectronics/cmsis_device_f1 $@

clean:
	$(RM) firmware.* cmsis_*
