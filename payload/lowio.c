#include "lowio.h"

#define PERVASIVERESET_BASE_ADDR	0xE3101000
#define PERVASIVEGATE_BASE_ADDR		0xE3102000

#define GPIO0_BASE_ADDR			0xE20A0000
#define GPIO1_BASE_ADDR			0xE0100000

#define GPIO_REGS(i)			((void *)((i) == 0 ? GPIO0_BASE_ADDR : GPIO1_BASE_ADDR))

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

void pervasive_clock_enable_uart(int bus)
{
	pervasive_mask_or(PERVASIVEGATE_BASE_ADDR + 0x120 + 4 * bus, 1);
}

void pervasive_reset_exit_uart(int bus)
{
	pervasive_mask_and_not(PERVASIVERESET_BASE_ADDR + 0x120 + 4 * bus, 1);
}

void pervasive_clock_enable_gpio(void)
{
	pervasive_mask_or(PERVASIVEGATE_BASE_ADDR + 0x100, 1);
}

void pervasive_reset_exit_gpio(void)
{
	pervasive_mask_and_not(PERVASIVERESET_BASE_ADDR + 0x100, 1);
}

void gpio_set_port_mode(int bus, int port, int mode)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);

	gpio_regs[0] = (gpio_regs[0] & ~(1 << port)) | (1 << port);

	asm volatile("dmb\n\t");
}

void gpio_port_set(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);

	gpio_regs[2] |= 1 << port;

	gpio_regs[0xD];

	asm volatile("dsb\n\t");
}

void gpio_port_clear(int bus, int port)
{
	volatile unsigned int *gpio_regs = GPIO_REGS(bus);

	gpio_regs[3] |= 1 << port;

	gpio_regs[0xD];

	asm volatile("dsb\n\t");
}
