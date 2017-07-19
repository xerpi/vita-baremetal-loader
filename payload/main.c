#include "lowio.h"
#include "uart.h"

static unsigned int get_cpu_id(void);

#define PRINT_HEX(x) \
	do { \
		for (int i = 28; i >= 0; i -= 4) \
			uart_write(0, "0123456789ABCDEF"[((x) >> i) & 0xF]); \
	} while (0);

int main(void)
{
	if (get_cpu_id() != 0) {
		while (1)
			;
	}

	pervasive_clock_enable_gpio();
	pervasive_reset_exit_gpio();

	gpio_set_port_mode(0, GPIO_PORT_GAMECARD_LED, GPIO_PORT_MODE_OUTPUT);
	gpio_port_set(0, GPIO_PORT_GAMECARD_LED);

	pervasive_clock_enable_uart(0);
	pervasive_reset_exit_uart(0);

	uart_init(0, 115200);

	pervasive_dsi_set_pixeclock(1, 0x223A1C);
	pervasive_clock_enable_dsi(1, 0xF);
	pervasive_reset_exit_dsi(1, 7);

	dsi_enable_bus(1);

	while (1) {
		static int i = 0;
		uart_write(0, '0' + (i++ % 10));
		uart_write(0, '\n');
		uart_write(0, '\r');

		if (i % 1000 < 500)
			gpio_port_set(0, GPIO_PORT_GAMECARD_LED);
		else
			gpio_port_clear(0, GPIO_PORT_GAMECARD_LED);
	}

	return 0;
}

static unsigned int get_cpu_id(void)
{
	unsigned int mpidr;
	asm volatile("mrc p15, 0, %0, c0, c0, 5\n\t" : "=r"(mpidr));
	return mpidr & 0xF;
}
