#ifndef LOWIO_H
#define LOWIO_H

void pervasive_clock_enable_uart(int bus);
void pervasive_reset_exit_uart(int bus);
void pervasive_clock_enable_gpio(void);
void pervasive_reset_exit_gpio(void);

#define GPIO_PORT_MODE_INPUT	0
#define GPIO_PORT_MODE_OUTPUT	1

#define GPIO_PORT_GAMECARD_LED	6
#define GPIO_PORT_PS_LED	7

void gpio_set_port_mode(int bus, int port, int mode);
void gpio_port_set(int bus, int port);
void gpio_port_clear(int bus, int port);

#endif
