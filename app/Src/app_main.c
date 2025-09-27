#include <stdint.h>
#include <string.h>

#include "usart.h"
#include "usbd_cdc_if.h"

#include "app_main.h"

#include "cli.h"
#include "cli_cmd.h"
#include "nmea.h"

#define UART_RX_BUF_SIZE RX_BUF_SIZE

// UART -> NMEA definitions
static uint8_t s_uart1_rx_buf[UART_RX_BUF_SIZE] = {0};

// USB CDC definitions
static uint8_t s_usb_rx_buf[CMD_RX_BUF_SIZE] = {0};
static uint8_t s_usb_tx_buf[CMD_TX_BUF_SIZE] = {0};

// UART->NMEA static functions
static void gps_uart_init();

void app_main() {
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

  while (1) {
    // process NMEA from uart
    nmea_process();

    // process needed tx for cli
    cli_process();
  }
}

//---- Callbacks to HAL and middlewares ----//

// GPS UART initialization
static void gps_uart_init() {
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_uart1_rx_buf, sizeof(s_uart1_rx_buf));
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
