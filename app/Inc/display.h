#ifndef _LAB2_DISPLAY_H
#define _LAB2_DISPLAY_H

#include <stdint.h>

#define SPI_TX_BUF_SIZE 256

typedef struct {
  void* spi_handle;

  void* gpio_port;
  uint16_t bl_pin;
  uint16_t cs_pin;
  uint16_t dc_pin;
  uint16_t rst_pin;
} display_init_config_t;

void display_init(const display_init_config_t* config);
void display_process();
void display_render_string(uint8_t start_x, uint8_t start_y, const char* str);
void display_set_window(uint8_t start_x, uint8_t start_y, uint8_t end_x, uint8_t end_y);

#endif