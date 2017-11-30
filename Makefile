TARGET   = baremetal-loader
TARGET_OBJS  = main.o trampoline.o
STAGE1_OBJS = stage1/stage1.o

LIBS =	-ltaihenForKernel_stub -lSceSysclibForDriver_stub -lSceSysmemForDriver_stub \
	-lSceSysmemForKernel_stub -lSceThreadmgrForDriver_stub  -lSceIofilemgrForDriver_stub \
	-lSceCpuForKernel_stub -lSceCpuForDriver_stub -lSceUartForKernel_stub \
	-lScePervasiveForDriver_stub -lSceSysconForDriver_stub -lScePowerForDriver_stub \
	-lSceSysrootForKernel_stub

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
AS      = $(PREFIX)-as
OBJCOPY = $(PREFIX)-objcopy
CFLAGS  = -Wl,-q -Wall -O0 -nostartfiles -mcpu=cortex-a9 -mthumb-interwork
ASFLAGS =

all: $(TARGET).skprx

%.skprx: %.velf
	vita-make-fself -c $< $@

%.velf: %.elf
	vita-elf-create -e $(TARGET).yml $< $@

stage1.elf: $(STAGE1_OBJS)
	$(CC) -T stage1/stage1.ld -nostartfiles -nostdlib $^ -o $@ -lgcc

stage1.bin: stage1.elf
	$(OBJCOPY) -S -O binary $^ $@

stage1_bin.o: stage1.bin
	$(OBJCOPY) --input binary --output elf32-littlearm \
		--binary-architecture arm $^ $@

$(TARGET).elf: $(TARGET_OBJS) stage1_bin.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

.PHONY: clean send

clean:
	@rm -rf $(TARGET).skprx $(TARGET).velf $(TARGET).elf $(TARGET_OBJS) \
		stage1.elf stage1.bin stage1_bin.o $(STAGE1_OBJS)

send: $(TARGET).skprx
	curl -T $(TARGET).skprx ftp://$(PSVITAIP):1337/ux0:/data/tai/kplugin.skprx
	@echo "Sent."
