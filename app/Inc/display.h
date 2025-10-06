#ifndef _LAB2_DISPLAY_H
#define _LAB2_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#define SPI_TX_BUF_SIZE 256

typedef struct {
  void *spi_handle;

  void *gpio_port;
  uint16_t bl_pin;
  uint16_t cs_pin;
  uint16_t dc_pin;
  uint16_t rst_pin;
} display_init_config_t;

void display_init(const display_init_config_t *config);
void display_callback(void *spi);
void display_set_window(uint16_t start_x, uint16_t start_y, uint16_t end_x,
                        uint16_t end_y);
void display_draw_rgb565(uint16_t *buf, size_t size);

#endif