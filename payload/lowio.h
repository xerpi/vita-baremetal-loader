#ifndef LOWIO_H
#define LOWIO_H

#define dmb() asm volatile("dmb\n\t")
#define dsb() asm volatile("dsb\n\t")

unsigned int pervasive_read_misc(unsigned int offset);
void pervasive_clock_enable_uart(int bus);
void pervasive_reset_exit_uart(int bus);
void pervasive_clock_enable_gpio(void);
void pervasive_reset_exit_gpio(void);
void pervasive_clock_enable_dsi(int bus, int value);
void pervasive_reset_exit_dsi(int bus, int value);
void pervasive_dsi_set_pixeclock(int bus, int pixelclock);

#define GPIO_PORT_MODE_INPUT	0
#define GPIO_PORT_MODE_OUTPUT	1

#define GPIO_PORT_GAMECARD_LED	6
#define GPIO_PORT_PS_LED	7

void gpio_set_port_mode(int bus, int port, int mode);
void gpio_port_set(int bus, int port);
void gpio_port_clear(int bus, int port);

void dsi_enable_bus(int bus);

#endif
