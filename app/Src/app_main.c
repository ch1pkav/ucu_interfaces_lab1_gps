#include <stdint.h>
#include <string.h>

#include "render.h"
#include "spi.h"
#include "stm32f4xx_hal.h"
#include "usart.h"
#include "usbd_cdc_if.h"

#include "app_main.h"

#include "cli.h"
#include "cli_cmd.h"
#include "display.h"
#include "nmea.h"

#define UART_RX_BUF_SIZE RX_BUF_SIZE

#define STATS_REFRESH_PERIOD_TICKS 100

// UART -> NMEA definitions
static uint8_t s_uart1_rx_buf[UART_RX_BUF_SIZE] = {0};

// USB CDC definitions
static uint8_t s_usb_rx_buf[CMD_RX_BUF_SIZE] = {0};
static uint8_t s_usb_tx_buf[CMD_TX_BUF_SIZE] = {0};

// UART->NMEA static functions
static void gps_uart_init();

// UID
uint32_t uid[3] = {0};

void app_main() {
  char buf[64];
  uid[0] = HAL_GetUIDw0();
  uid[1] = HAL_GetUIDw1();
  uid[2] = HAL_GetUIDw2();
  sprintf(buf, "MCUID: %x%x%x%x-%x%x%x%x-%x%x%x%x",
          (uint8_t)(uid[0] >> 24) & 0xFF, (uint8_t)(uid[0] >> 16) & 0xFF,
          (uint8_t)(uid[0] >> 8) & 0xFF, (uint8_t)(uid[0] & 0xFF),
          (uint8_t)(uid[1] >> 24) & 0xFF, (uint8_t)(uid[1] >> 16) & 0xFF,
          (uint8_t)(uid[1] >> 8) & 0xFF, (uint8_t)(uid[1] & 0xFF),
          (uint8_t)(uid[2] >> 24) & 0xFF, (uint8_t)(uid[2] >> 16) & 0xFF,
          (uint8_t)(uid[2] >> 8) & 0xFF, (uint8_t)(uid[2] & 0xFF));

  gps_uart_init();

  {
    size_t map_size;
    const cli_cmd_entry_t *cmd_map = cli_cmd_get_map(&map_size);

    cli_init_config_t cli_config = {.rx_buf = s_usb_rx_buf,
                                    .tx_buf = s_usb_tx_buf,
                                    .cmd_map = cmd_map,
                                    .cmd_map_len = map_size,
                                    .tx_func = &CDC_Transmit_FS};

    cli_init(&cli_config);
  }

  {
    const display_init_config_t display_config = {&hspi1,      DISP_GPIO_PORT,
                                                  DISP_BL_Pin, DISP_CS_Pin,
                                                  DISP_DC_Pin, DISP_RESET_Pin};

    display_init(&display_config);
  }

  // Render MCUID
  render_set_row(4);
  render_text(buf);

  size_t start_tick = HAL_GetTick();
  size_t i = 0;

  while (1) {
    // process NMEA from uart
    nmea_process();

    // process needed tx for cli
    cli_process();

    if (HAL_GetTick() - start_tick >= STATS_REFRESH_PERIOD_TICKS) {
      sprintf(buf, "%d", ++i);
      render_set_row(0);
      render_text(buf);

      start_tick = HAL_GetTick();
    }
  }
}

//---- Callbacks to HAL and middlewares ----//

// GPS UART initialization
static void gps_uart_init() {
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_uart1_rx_buf, sizeof(s_uart1_rx_buf));
  const char *req = "$CCGNQ,GGA\r\n";
  HAL_UART_Transmit(&huart1, (const uint8_t *)req, strlen(req), UINT32_MAX);
}

// GPS UART handler
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  nmea_rx_callback(s_uart1_rx_buf, Size);
}

// USB CDC log handler
void USBD_CDC_AppHandler(uint8_t *buf, size_t len) {
  cli_print(buf, len);
  cli_rx_callback(buf, len);
}

// printf implementation
int _write(int file, char *ptr, int len) {
  cli_print((uint8_t *)ptr, len);
  return len;
}

// display SPI callback
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) { display_callback(hspi); }
