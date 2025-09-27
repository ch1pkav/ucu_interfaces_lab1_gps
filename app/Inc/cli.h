#ifndef _LAB1_CLI_H
#define _LAB1_CLI_H

#include "forward.h"
#include <stdint.h>

#define CMD_MAX_STRLEN 32
#define CMD_BUF_SIZE CMD_MAX_STRLEN * 4
typedef bool_t(*cmd_cli_cb_t)(const char*);


typedef struct {
  char cmd[CMD_MAX_STRLEN];
  cmd_cli_cb_t callback;
} cli_cmd_entry_t;

typedef struct {
  volatile uint8_t* rx_buf;
  const cli_cmd_entry_t* cmd_map;
  size_t cmd_map_len;
} cli_init_config_t;

void cli_init(cli_init_config_t *config);
void cli_rx_callback(const uint8_t* buf, size_t size);
void cli_process();

#endif //_LAB1_CLI_H