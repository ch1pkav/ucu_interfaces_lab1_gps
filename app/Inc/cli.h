#ifndef _LAB1_CLI_H
#define _LAB1_CLI_H

#include "forward.h"
#include <stdint.h>

#define CMD_MAX_STRLEN 32
#define CMD_RX_BUF_SIZE (CMD_MAX_STRLEN * 4)
#define CMD_TX_BUF_SIZE 512

typedef bool_t(*cmd_cli_cb_t)(const char*);
typedef uint8_t(*cmd_cli_tx_t)(uint8_t*, uint16_t);

typedef struct {
  char cmd[CMD_MAX_STRLEN];
  cmd_cli_cb_t callback;
} cli_cmd_entry_t;

typedef struct {
  volatile uint8_t* rx_buf;
  volatile uint8_t* tx_buf;
  cmd_cli_tx_t tx_func;
  const cli_cmd_entry_t* cmd_map;
  size_t cmd_map_len;

} cli_init_config_t;

void cli_init(cli_init_config_t *config);
void cli_rx_callback(const uint8_t* buf, size_t size);
void cli_process();
void cli_print(const uint8_t* ptr, size_t len);

#endif //_LAB1_CLI_H