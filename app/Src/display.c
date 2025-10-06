#include <stddef.h>
#include <stdint.h>

#include "forward.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"

#include "display.h"
#include "font.h"
#include "spi.h"
#include "st7735_reg.h"

static display_init_config_t s_config;
static uint8_t s_tx_buf_storage[SPI_TX_BUF_SIZE];
static volatile size_t s_spi_last_sent = 0;

RINGBUF_DECLARE(s_spi_tx, s_tx_buf_storage);

bool_t tx_ready() {
  return (HAL_SPI_STATE_READY == HAL_SPI_GetState(s_config.spi_handle));
}

void wait_for_tx_ready() {
  while (!tx_ready()) {
  };
}

// ----FIFO operations---- //
void push_fifo_data(const uint8_t *buf, size_t size) {
  RINGBUF_ISR_COPY(s_spi_tx, SPI_TX_BUF_SIZE);
}

void write_reg(uint8_t reg, const uint8_t *buf, size_t size) {
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin,
                    0); // TODO: Synchronize pins with DMA ops
  push_fifo_data(&reg, 1);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin, 1);
  push_fifo_data(buf, size);
}

// ----Blocking direct operations---- //
void transfer_data(const uint8_t *buf, size_t size) {
  HAL_SPI_Transmit(s_config.spi_handle, buf, size, UINT32_MAX);
}

void write_reg_blocking(uint8_t reg, const uint8_t *buf, size_t size) {
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin, 0);
  transfer_data(&reg, 1);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin, 1);
  transfer_data(buf, size);
}

void spi_tft_init_procedure() {
  uint8_t tmp = 0x00;

  // GPIO RST
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.rst_pin, 0);
  HAL_Delay(12);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.rst_pin, 1);
  HAL_Delay(12);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.cs_pin, 1);

  // SLPOUT
  tmp = 0x00;
  write_reg_blocking(ST7735_SLEEP_OUT, &tmp, 1);
  HAL_Delay(120);

  // COLMOD
  tmp = 0x55;
  write_reg_blocking(ST7735_COLOR_MODE, &tmp, 1);

  // MADCTL

  // FRMCTR, GAMSET, INVON/INVOFF

  // DISPON

  // BL
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.bl_pin, 1);
}

// -----Public interface------ //
void display_init(const display_init_config_t *config) {
  s_config = *config;
  spi_tft_init_procedure();
}

void display_process() {
  if (s_spi_tx_data_ready && (tx_ready())) {
    if (s_spi_tx_buf_offset > s_spi_last_sent) {
      HAL_SPI_Transmit_DMA(s_config.spi_handle,
                           (void *)&s_spi_tx_buf[s_spi_last_sent],
                           s_spi_tx_buf_offset - s_spi_last_sent);
      s_spi_last_sent = s_spi_tx_buf_offset;
    } else {
      HAL_SPI_Transmit_DMA(s_config.spi_handle,
                           (void *)&s_spi_tx_buf[s_spi_last_sent],
                           SPI_TX_BUF_SIZE - s_spi_last_sent);
      s_spi_last_sent = 0;
    }
    s_spi_tx_data_ready = b_FALSE;
  }
}

void display_render_string(uint8_t start_x, uint8_t start_y, const char *str) {}

void display_set_window(uint8_t start_x, uint8_t start_y, uint8_t end_x,
                        uint8_t end_y) {}