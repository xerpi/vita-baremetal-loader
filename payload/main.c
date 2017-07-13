static inline unsigned long get_cpu_id(void)
{
	unsigned long mpidr;
	asm volatile("mrc p15, 0, %0, c0, c0, 5\n\t" : "=r"(mpidr));
	return mpidr & 7;
}

#define PERVASIVERESET_BASE_ADDR	0xE3101000
#define PERVASIVEGATE_BASE_ADDR		0xE3102000

static inline void pervasive_mask_or(unsigned int addr, unsigned int val)
{
	volatile unsigned long tmp;

	asm volatile(
		"ldr %0, [%1]\n\t"
		"orr %0, %2\n\t"
		"str %0, [%1]\n\t"
		"dmb\n\t"
		"ldr %0, [%1]\n\t"
		"dsb\n\t"
		: "=&r"(tmp)
		: "r"(addr), "r"(val)
	);
}

static inline void pervasive_mask_and_not(unsigned int addr, unsigned int val)
{
	volatile unsigned long tmp;

	asm volatile(
		"ldr %0, [%1]\n\t"
		"bic %0, %2\n\t"
		"str %0, [%1]\n\t"
		"dmb\n\t"
		"ldr %0, [%1]\n\t"
		"dsb\n\t"
		: "=&r"(tmp)
		: "r"(addr), "r"(val)
	);
}

static void pervasive_clock_enable_uart(int bus)
{
	pervasive_mask_or(PERVASIVEGATE_BASE_ADDR + 0x120 + 4 * bus, 1);
}

static void pervasive_reset_exit_uart(int bus)
{
	pervasive_mask_and_not(PERVASIVERESET_BASE_ADDR + 0x120 + 4 * bus, 1);
}

static void pervasive_clock_enable_gpio(void)
{
	pervasive_mask_or(PERVASIVEGATE_BASE_ADDR + 0x100, 1);
}

static void pervasive_reset_exit_gpio(void)
{
	pervasive_mask_and_not(PERVASIVERESET_BASE_ADDR + 0x100, 1);
}

#define UART_REG_BASE_ADDR		0xE2030000
#define UARTCLKGEN_REG_BASE_ADDR	0xE3105000

#define UART_REGS(i) ((void *)(UART_REG_BASE_ADDR + (i) * 0x10000))
#define UARTCLKGEN_REGS(i) ((void *)(UARTCLKGEN_REG_BASE_ADDR + (i) * 4))

static unsigned int uart_read_fifo_data_available(int bus)
{
	return ((unsigned int *)UART_REGS(bus))[0x1A] & 0x3F;
}

static void uart_wait_ready(int bus)
{
	volatile unsigned int *ptr = UART_REGS(bus);
        while (!(ptr[0xA] & 0x200))
		asm volatile("dmb sy\n\t");
}

static void uart_init(int bus)
{
	volatile unsigned int *uart_regs = UART_REGS(bus);
	volatile unsigned int *uartclkgen_regs = UARTCLKGEN_REGS(bus);

	uart_regs[1] = 0; // disable device

	*uartclkgen_regs = 0x1001A; // Baudrate = 115200

	uart_regs[8] = 3;
	uart_regs[4] = 1;
	uart_regs[0xC] = 0;
	uart_regs[0x18] = 0x303;
	uart_regs[0x10] = 0;
	uart_regs[0x14] = 0;
	uart_regs[0x19] = 0x10001;

	uart_regs[1] = 1; // enable device

	uart_wait_ready(bus);
}

static unsigned int uart_read(int bus)
{
        unsigned int result;

	volatile unsigned int *ptr = UART_REGS(bus);

	while (!(ptr[0x1A] << 0x1A))
		asm volatile("dmb sy\n\t");
        result = ptr[0x1E];
        ptr[0x15] = 0x77F;

        return result;
}

static void uart_putch(int bus, unsigned int data)
{
	volatile unsigned int *ptr = UART_REGS(bus);

	while (!(ptr[0xA] & 0x100))
		asm volatile("dmb sy\n\t");

        ptr[0x1C] = data;
}

static void uart_print(int bus, const char *str)
{
	while (*str)
		uart_putch(bus, *str++);
}

int main(void)
{
	if (get_cpu_id() != 0) {
		while (1)
			;
	}

	pervasive_clock_enable_gpio();
	pervasive_reset_exit_gpio();

	*(volatile unsigned int *)(0xE20A0000 + 0x00) |= (1 << 7) | (1 << 6); // Port mode: output
	*(volatile unsigned int *)(0xE20A0000 + 0x08) = (1 << 7) | (1 << 6); // Port set
	volatile unsigned int tmp = *(volatile unsigned int *)(0xE20A0000 + 0x34);
	asm volatile("dsb sy\n\t");

	/* *(volatile unsigned int *)(0xE0100000 + 0x00) = 0xFFFFFFFF; // Port mode: output
	*(volatile unsigned int *)(0xE0100000 + 0x08) = 0xFFFFFFFF; // Port set
	volatile unsigned int tmp2 = *(volatile unsigned int *)(0xE0100000 + 0x34);
	asm volatile("dsb sy\n\t");*/

	pervasive_clock_enable_uart(0);
	pervasive_reset_exit_uart(0);

	uart_init(0);

	while (1) {
		static int i = 0;
		uart_print(0, "Vita ");
		uart_putch(0, '0' + (i++ % 10));
		uart_print(0, "\n\r");

		if (i % 1000 < 500)
			*(volatile unsigned int *)(0xE20A0000 + 0x08) = (1 << 7) | (1 << 6); // Port set
		else
			*(volatile unsigned int *)(0xE20A0000 + 0x0C) = (1 << 7) | (1 << 6); // Port clear
		asm volatile("dsb sy\n\t");
	}

	return 0;
}
