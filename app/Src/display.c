#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "forward.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_spi.h"

#include "display.h"
#include "ili9341_defs.h"

#define CMD_FIFO_LEN 4
#define CMD_MAX_DATA_SIZE 256

typedef struct {
  uint8_t cmd;
  uint8_t data[CMD_MAX_DATA_SIZE];
  size_t data_size;
} spi_cmd_t;

typedef enum {
  ets_CMD = 0,
  ets_DATA = 1,
  ets_NONE = 2,
} tx_state;

static const spi_cmd_t null_cmd = {0};

static display_init_config_t s_config = {0};
static tx_state s_tx_state = ets_CMD;
static spi_cmd_t s_cmd_fifo[CMD_FIFO_LEN] = {0};
static size_t s_cur_cmd = 0;
static size_t s_fifo_head = 0;

void set_cmd() {
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin,
                    (GPIO_PinState)ets_CMD);
  s_tx_state = ets_CMD;
}

void set_data() {
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.dc_pin,
                    (GPIO_PinState)ets_DATA);
  s_tx_state = ets_DATA;
}

// ----Blocking direct operations for init---- //
void transfer_data_blocking(const uint8_t *buf, size_t size) {
  HAL_SPI_Transmit(s_config.spi_handle, buf, size, UINT32_MAX);
}

void write_reg_blocking(uint8_t reg, const uint8_t *buf, size_t size) {
  set_cmd();
  transfer_data_blocking(&reg, 1);
  set_data();
  transfer_data_blocking(buf, size);
}

void spi_tft_init_procedure() {
  uint8_t tmp = 0x00;

  // GPIO RST
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.rst_pin, 0);
  HAL_Delay(120);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.rst_pin, 1);
  HAL_Delay(120);
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.cs_pin, 1);
  HAL_Delay(120);

  // SWRST
  tmp = 0x00;
  write_reg_blocking(ILI9341_SWRESET, &tmp, 1);

  // SLPOUT
  tmp = 0x00;
  write_reg_blocking(ILI9341_SLPOUT, &tmp, 1);
  HAL_Delay(150);

  // COLMOD
  tmp = 0x55;
  write_reg_blocking(ILI9341_PIXFMT, &tmp, 1);

  // MADCTL
  tmp = 0x00;
  write_reg_blocking(ILI9341_MADCTL, &tmp, 1);

  // FRMCTR
  uint8_t frmctr[2] = {0x00, 0x1B};
  write_reg_blocking(ILI9341_FRMCTR1, frmctr, 2);

  // GAMSET
  tmp = 0x01;
  write_reg_blocking(ILI9341_GAMMASET, &tmp, 1);

  // INVOFF
  tmp = 0x00;
  write_reg_blocking(ILI9341_INVOFF, &tmp, 1);

  // DISPON
  write_reg_blocking(ILI9341_DISPON, &tmp, 1);

  // BL
  HAL_GPIO_WritePin(s_config.gpio_port, s_config.bl_pin, 1);
}

err_t queue_cmd(const spi_cmd_t *cmd) {
  if (memcmp(&null_cmd, &s_cmd_fifo[s_fifo_head], sizeof(spi_cmd_t))) {
    return err_BUSY;
  }

  memcpy(&s_cmd_fifo[s_fifo_head], cmd, sizeof(spi_cmd_t));

  if (s_tx_state == ets_NONE) {
    set_cmd();
    HAL_SPI_Transmit_DMA(s_config.spi_handle, &s_cmd_fifo[s_fifo_head].cmd, 1);
  }

  s_fifo_head = (s_fifo_head + 1) % CMD_FIFO_LEN;

  return err_OK;
}


// -----Public interface------ //
void display_init(const display_init_config_t *config) {
  s_config = *config;
  spi_tft_init_procedure();
}

void display_callback(void *spi) {
  if (s_config.spi_handle == spi) {

    switch (s_tx_state) {
    case ets_CMD:
      if (memcmp(&s_cmd_fifo[s_cur_cmd], &null_cmd, sizeof(null_cmd)) == 0) {
        // no pending command
        s_tx_state = ets_NONE;
        return;
      }

      // send data
      set_data();
      HAL_SPI_Transmit_DMA(spi, s_cmd_fifo[s_cur_cmd].data,
                           s_cmd_fifo[s_cur_cmd].data_size);
      break;
    case ets_DATA:
      // cleanup last cmd
      memset(&s_cmd_fifo[s_cur_cmd], 0x00, sizeof(spi_cmd_t));
      s_cur_cmd = (s_cur_cmd + 1) % CMD_FIFO_LEN;

      // send next cmd
      set_cmd();
      HAL_SPI_Transmit_DMA(spi, &s_cmd_fifo[s_cur_cmd].cmd, 1);
      break;
    default:
      break;
    }
  }
}

void display_render_string(uint8_t start_x, uint8_t start_y, const char *str) {}

void display_set_window(uint16_t start_x, uint16_t start_y, uint16_t end_x,
                        uint16_t end_y) {
  spi_cmd_t tmp_cmd = {ILI9341_CASET,
                       {start_x >> 8, start_x & 0xFF, end_x >> 8, end_x & 0xFF},
                       4};

  queue_cmd(&tmp_cmd);

  tmp_cmd.cmd = ILI9341_PASET;
  tmp_cmd.data[0] = start_x >> 8;
  tmp_cmd.data[1] = start_x & 0xFF;

  tmp_cmd.data[2] = start_y >> 8;
  tmp_cmd.data[3] = start_y & 0xFF;

  queue_cmd(&tmp_cmd);
}

void display_draw_rgb565(uint16_t* buf, size_t size) {
  spi_cmd_t tmp_cmd = {
    ILI9341_RAMWR,
    {},
    size,
  };

  memcpy(tmp_cmd.data, buf, size * sizeof(uint16_t));
  queue_cmd(&tmp_cmd);
}