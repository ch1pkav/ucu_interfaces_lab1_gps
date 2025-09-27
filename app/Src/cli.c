#include "cli.h"
#include "forward.h"
#include "nmea.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const cli_cmd_entry_t *s_cmd_map;
static size_t s_cmd_map_len;
static cmd_cli_tx_t s_tx_fn;

RINGBUF_DECLARE(s_cli_rx, NULL);
RINGBUF_DECLARE(s_cli_tx, NULL);

size_t s_cli_tx_last_sent = 0;

void cli_init(cli_init_config_t *config) {
  s_cli_rx_buf = config->rx_buf;
  s_cli_tx_buf = config->tx_buf;
  s_tx_fn = config->tx_func;

  memset((void *)s_cli_rx_buf, 0x00, CMD_RX_BUF_SIZE);
  memset((void *)s_cli_tx_buf, 0x00, CMD_TX_BUF_SIZE);

  s_cmd_map = config->cmd_map;
  s_cmd_map_len = config->cmd_map_len;
}

static const uint8_t *find_cmd_end(const uint8_t *begin, const uint8_t *end) {
  for (const uint8_t *ptr = begin; ptr < end - 1; ptr += 1) {
    if (*ptr == '\r') {
      return (ptr + 1);
    }
  }

  return NULL;
}

static size_t get_cmd(const uint8_t *in_buf, const uint8_t *data_begin,
                      const uint8_t *data_end, char *out_buf) {
  size_t cmd_len = 0;

  // handle buffer overflow
  if (data_end > data_begin) {
    cmd_len = data_end - data_begin;
    memcpy(out_buf, data_begin, cmd_len);
  } else {
    size_t start_len = &in_buf[NMEA_PARSER_BUF_SIZE] - data_begin;
    size_t end_len = data_end - in_buf;
    cmd_len = start_len + end_len;

    memcpy(out_buf, data_begin, start_len);
    memcpy(&out_buf[start_len], in_buf, end_len);
  }

  out_buf[cmd_len] = '\0';

  return cmd_len;
}

void cli_rx_process() {
  static char cmd[CMD_MAX_STRLEN] = {0};
  static const uint8_t *s_last_cmd_end = NULL;
  const uint8_t *data_end = (const uint8_t *)&(
      s_cli_rx_buf[(s_cli_rx_buf_offset + s_cli_rx_last_chunk_size) %
                   CMD_RX_BUF_SIZE]);

  if (!s_cli_rx_data_ready) {
    return;
  }

  if (s_last_cmd_end == NULL) {
    s_last_cmd_end = (void *)s_cli_rx_buf;
  }

  const uint8_t *cmd_begin = s_last_cmd_end;

  s_cli_rx_data_ready = b_FALSE;

  // handle buffer overflow
  if (data_end >= cmd_begin) {
    s_last_cmd_end = find_cmd_end(cmd_begin, data_end);
  } else {
    s_last_cmd_end = find_cmd_end(
        cmd_begin, (const uint8_t *)&s_cli_rx_buf[CMD_RX_BUF_SIZE]);

    if (s_last_cmd_end == NULL) {
      s_last_cmd_end = find_cmd_end((const uint8_t *)s_cli_rx_buf, data_end);
    }
  }

  // no new valid sentence
  if (s_last_cmd_end == NULL) {
    s_last_cmd_end = cmd_begin;
    return;
  }

  get_cmd((const uint8_t *)s_cli_rx_buf, cmd_begin, s_last_cmd_end, cmd);
  printf("\r\n");

  for (size_t i = 0; i < s_cmd_map_len; ++i) {
    if (strstr(cmd, (s_cmd_map + i)->cmd) && (s_cmd_map + i)->callback(cmd)) {
      break;
    }
  }
}

void cli_tx_process() {
  if (s_cli_tx_data_ready && (0 == s_tx_fn(NULL, 0))) {
    if (s_cli_tx_buf_offset > s_cli_tx_last_sent) {
      s_tx_fn((void *)&s_cli_tx_buf[s_cli_tx_last_sent],
              s_cli_tx_buf_offset - s_cli_tx_last_sent);
      s_cli_tx_last_sent = s_cli_tx_buf_offset;
    } else {
      s_tx_fn((void *)&s_cli_tx_buf[s_cli_tx_last_sent],
              CMD_TX_BUF_SIZE - s_cli_tx_last_sent);
      s_cli_tx_last_sent = 0;
    }
    s_cli_tx_data_ready = b_FALSE;
  }
}

void cli_process() {
  cli_rx_process();
  cli_tx_process();
}

void cli_rx_callback(const uint8_t *buf, size_t size) {
  RINGBUF_ISR_COPY(s_cli_rx, CMD_RX_BUF_SIZE);
}

void cli_print(const uint8_t *buf, size_t size) {
  RINGBUF_ISR_COPY(s_cli_tx, CMD_TX_BUF_SIZE);
}