#include "nmea.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "forward.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

// Parser definitions
static nmea_sentence_t s_nmea_last_sentence = {0};

// I/O ringbuf defienitions
static volatile uint8_t s_nmea_parser_buf_storage[NMEA_PARSER_BUF_SIZE] = {0};

RINGBUF_DECLARE(s_nmea_parser, s_nmea_parser_buf_storage);

// Raw toggle
static bool_t s_raw = b_FALSE;

// Stale?
static size_t s_last_update = 0;
#define TICKS_TO_STALE 10000

// NMEA parsing
static const uint8_t *nmea_sentence_find_end(const uint8_t *begin,
                                             const uint8_t *end) {
  for (const uint8_t *ptr = begin; ptr < end - 1; ptr += 1) {
    if (*ptr == '\r' && *(ptr + 1) == '\n') {
      return (ptr + 2);
    }
  }

  return NULL;
}

static size_t nmea_sentence_get(const uint8_t *in_buf,
                                const uint8_t *data_begin,
                                const uint8_t *data_end, char *out_buf) {
  size_t sentence_len = 0;

  // handle buffer overflow
  if (data_end > data_begin) {
    sentence_len = data_end - data_begin;
    memcpy(out_buf, data_begin, sentence_len);
  } else {
    size_t start_len = &in_buf[NMEA_PARSER_BUF_SIZE] - data_begin;
    size_t end_len = data_end - in_buf;
    sentence_len = start_len + end_len;

    memcpy(out_buf, data_begin, start_len);
    memcpy(&out_buf[start_len], in_buf, end_len);
  }

  out_buf[sentence_len] = '\0';

  return sentence_len;
}

static nmea_timestamp_t parse_timestamp(const char *token, char *parse_buf) {
  nmea_timestamp_t res = {0};
  const uint8_t step = 2;
  parse_buf[step] = 0;

  memcpy(parse_buf, token, step);
  res.hr = atoi(parse_buf);

  memcpy(parse_buf, token + step, step);
  res.min = atoi(parse_buf);

  memcpy(parse_buf, token + 2 * step, step);
  res.sec = atoi(parse_buf);

  strcpy(parse_buf + 1, &token[step * 3]);
  parse_buf[step * 2] = '\0';
  parse_buf[0] = '0';
  res.subsec = strtod(parse_buf, NULL);

  return res;
}

static nmea_pos_t parse_pos(const char *token, char *parse_buf, size_t step) {
  nmea_pos_t res = {0};

  memcpy(parse_buf, token, step);
  parse_buf[step] = '\0';
  res.deg = atoi(parse_buf);

  strcpy(parse_buf, token + step);
  res.min = strtod(parse_buf, NULL);
  return res;
}

static err_t nmea_sentence_parse_token(nmea_token_type_t token_type,
                                       const char *token,
                                       nmea_sentence_t *out) {
  static char parse_buf[NMEA_PARSER_DEFAULT_TOKEN_SIZE] = {0};
  switch (token_type) {
  case entt_TIMESTAMP:
    out->timestamp = parse_timestamp(token, parse_buf);
    break;

  case entt_LAT:
    out->lat = parse_pos(token, parse_buf, enpps_LAT);
    break;

  case entt_CARDINAL_LAT:
    out->lat.dir = *token;
    break;

  case entt_LONG:
    out->lon = parse_pos(token, parse_buf, enpps_LON);
    break;

  case entt_CARDINAL_LONG:
    out->lon.dir = *token;
    break;

  case entt_QUALITY:
    out->quality = atoi(token);
    break;

  case entt_BEGIN:
    if (strstr(token, "GGA") == NULL) {
      return err_AGAIN;
    }
    break;

  default:
    break;
  }
  return err_OK;
}

static err_t nmea_sentence_parse(char *sentence, nmea_sentence_t *out) {
  const char *token = strtok(sentence, ",");
  bool_t gga = b_TRUE;
  for (uint8_t token_num = 0; token; ++token_num) {
    if (gga && nmea_sentence_parse_token(token_num, token, out) == err_AGAIN) {
      gga = b_FALSE;
    }
    token = strtok(NULL, ",");
  }

  if (gga) {
    s_last_update = HAL_GetTick();
  }

  return err_OK;
}

static err_t nmea_raw_parse(const uint8_t *buf, size_t offset, size_t size) {
  static char s_sentence_buf[NMEA_PARSER_MAX_SENTENCE_SIZE] = {0};
  static const uint8_t *s_last_sentence_end = (void *)s_nmea_parser_buf_storage;
  const uint8_t *sentence_begin = s_last_sentence_end;
  const uint8_t *data_end = &(buf[(offset + size) % NMEA_PARSER_BUF_SIZE]);
  if (sentence_begin == NULL) {
    s_last_sentence_end = buf;
    return err_AGAIN;
  }

  // handle buffer overflow
  if (data_end >= sentence_begin) {
    s_last_sentence_end = nmea_sentence_find_end(sentence_begin, data_end);
  } else {
    s_last_sentence_end =
        nmea_sentence_find_end(sentence_begin, &buf[NMEA_PARSER_BUF_SIZE]);

    if (s_last_sentence_end == NULL) {
      s_last_sentence_end = nmea_sentence_find_end(buf, data_end);
    }
  }

  if (*sentence_begin != '$') {
    return err_AGAIN;
  }

  // no new valid sentence
  if (s_last_sentence_end == NULL) {
    s_last_sentence_end = sentence_begin;
    return err_NOTFOUND;
  }

  nmea_sentence_get(buf, sentence_begin, s_last_sentence_end, s_sentence_buf);

  return nmea_sentence_parse(s_sentence_buf, &s_nmea_last_sentence);
}

// I/O
void nmea_rx_callback(const uint8_t *buf, size_t size) {
  if (s_raw) {
    RAW_PRINT(buf, size);
  }
  BLINK_LED();

  RINGBUF_ISR_COPY(s_nmea_parser, NMEA_PARSER_BUF_SIZE);
}

// Public interface
void nmea_process() {
  if (s_nmea_parser_data_ready) {
    while (nmea_raw_parse((const uint8_t *)s_nmea_parser_buf,
                          s_nmea_parser_buf_offset,
                          s_nmea_parser_last_chunk_size) != err_NOTFOUND) {
    }

    s_nmea_parser_data_ready = b_FALSE;
  }

  s_nmea_last_sentence.stale =
      (s_nmea_last_sentence.stale ||
       ((HAL_GetTick() - s_last_update) > TICKS_TO_STALE));
}

const nmea_sentence_t *nmea_get_last_sentence() {
  return &s_nmea_last_sentence;
}

bool_t nmea_set_raw(bool_t on) {
  s_raw = on;

  return b_TRUE;
}
