TARGET   = baremetal-loader
TARGET_OBJS  = main.o resume.o
BOOTSTRAP_OBJS = payload_bootstrap.o

COMMON_LIBS = -ltaihenForKernel_stub -lSceSysclibForDriver_stub -lSceSysmemForDriver_stub \
	-lSceThreadmgrForDriver_stub -lSceCpuForDriver_stub -lScePervasiveForDriver_stub \
	-lSceSysconForDriver_stub -lScePowerForDriver_stub -lSceIofilemgrForDriver_stub \
	-lSceSysrootForKernel_stub

LIBS = $(COMMON_LIBS) -lSceSysmemForKernel_stub -lSceCpuForKernel_stub -lSceUartForKernel_stub
LIBS_363 = $(COMMON_LIBS) -lSceSysmemForKernel_363_stub -lSceCpuForKernel_363_stub -lSceUartForKernel_363_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
AS      = $(PREFIX)-as
OBJCOPY = $(PREFIX)-objcopy
CFLAGS  = -Wl,-q -Wall -O0 -nostartfiles -mcpu=cortex-a9 -mthumb-interwork
ASFLAGS =

all: $(TARGET).skprx $(TARGET)_363.skprx

%.skprx: %.velf
	vita-make-fself -c $< $@

%.velf: %.elf
	vita-elf-create -e $(TARGET).yml $< $@

payload_bootstrap.elf: $(BOOTSTRAP_OBJS)
	$(CC) -T payload_bootstrap.ld -nostartfiles -nostdlib $^ -o $@ -lgcc

payload_bootstrap.bin: payload_bootstrap.elf
	$(OBJCOPY) -S -O binary $^ $@

payload_bootstrap_bin.o: payload_bootstrap.bin
	$(OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm $^ $@

$(TARGET).elf: $(TARGET_OBJS) payload_bootstrap_bin.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

$(TARGET)_363.elf: $(TARGET_OBJS) payload_bootstrap_bin.o
	$(CC) $(CFLAGS) $^ $(LIBS_363) -o $@

.PHONY: all clean send

clean:
	@rm -rf $(TARGET).skprx $(TARGET).velf $(TARGET).elf \
	$(TARGET)_363.skprx $(TARGET)_363.velf $(TARGET)_363.elf \
	$(TARGET_OBJS) $(BOOTSTRAP_OBJS) \
	payload_bootstrap.elf payload_bootstrap.bin payload_bootstrap_bin.o

send: $(TARGET).skprx
	curl --ftp-method nocwd -T $(TARGET).skprx ftp://$(PSVITAIP):1337/ux0:/data/tai/kplugin.skprx
	@echo "Sent."
