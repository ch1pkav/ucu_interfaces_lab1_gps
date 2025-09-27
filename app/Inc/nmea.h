#ifndef _LAB1_NMEA_H
#define _LAB1_NMEA_H

#include "forward.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
  err_OK,
  err_CHECKSUM,
  err_AGAIN,
} nmea_err_t;

typedef enum {
  entt_BEGIN = 0,
  entt_TIMESTAMP,
  entt_LAT,
  entt_CARDINAL_LAT,
  entt_LONG,
  entt_CARDINAL_LONG,
  entt_QUALITY,
  entt_LAST = 15
} nmea_token_type_t;

typedef enum {
  enpps_LAT = 2,
  enpps_LON = 3,
} nmea_pos_parsing_step_t;

typedef char nmea_cardinal_t;

#define NMEA_CARDINAL_DIR_NORTH		(nmea_cardinal_t) 'N'
#define NMEA_CARDINAL_DIR_EAST		(nmea_cardinal_t) 'E'
#define NMEA_CARDINAL_DIR_SOUTH		(nmea_cardinal_t) 'S'
#define NMEA_CARDINAL_DIR_WEST		(nmea_cardinal_t) 'W'
#define NMEA_CARDINAL_DIR_UNKNOWN	(nmea_cardinal_t) '\0'

typedef enum {
  enq_UNKNOWN = 1,
  enq_DIFF = 2,
  enq_RTKFIX = 3,
  enq_RTKFLOAT = 4,
} nmea_quality_t;

typedef struct {
  nmea_cardinal_t dir;
  uint8_t deg;
  double min;
} nmea_pos_t;

typedef struct {
  uint8_t hr;
  uint8_t min;
  uint8_t sec;
  double subsec;
} nmea_timestamp_t;

typedef struct {
  bool_t stale;
  nmea_timestamp_t timestamp;
  nmea_pos_t lat;
  nmea_pos_t lon;
  nmea_quality_t quality;
} nmea_sentence_t;

#ifndef RX_BUF_SIZE
#define RX_BUF_SIZE 512
#endif

#define NMEA_PARSER_QUEUE_LEN 4
#define NMEA_PARSER_BUF_SIZE (NMEA_PARSER_QUEUE_LEN * RX_BUF_SIZE)
#define NMEA_PARSER_MAX_SENTENCE_SIZE (RX_BUF_SIZE / 2)
#define NMEA_PARSER_DEFAULT_TOKEN_SIZE 32

void nmea_rx_callback(const uint8_t *buf, size_t size);

void nmea_process();

bool_t nmea_set_raw(bool_t on);

const nmea_sentence_t* nmea_get_last_sentence();

#endif // ifndef _LAB1_NMEA_H