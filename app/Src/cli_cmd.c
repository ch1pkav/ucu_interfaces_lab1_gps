#include "cli_cmd.h"
#include "forward.h"
#include "nmea.h"
#include <stdio.h>

// Forward decl
static bool_t get_ver(const char *);
static bool_t get_pos(const char *);
static bool_t get_time(const char *);
static bool_t raw_on(const char *);
static bool_t raw_off(const char *);

static const cli_cmd_entry_t s_cli_cmd_map[] = {{"VER", &get_ver},
                                                {"GET POS", &get_pos},
                                                {"GET TIME", &get_time},
                                                {"RAW ON", &raw_on},
                                                {"RAW OFF", &raw_off}};

#define MAP_SIZE (sizeof(s_cli_cmd_map) / sizeof(cli_cmd_entry_t))

static bool_t get_ver(const char *cmd) {
  UNUSED(cmd);

  printf("ver,\r\n"
         "lab1,\r\n");

  return b_TRUE;
}

static bool_t get_pos(const char *cmd) {
  UNUSED(cmd);

  const nmea_sentence_t *sentence = nmea_get_last_sentence();

  printf("dir,deg,min,card,\r\n"
         "LAT,%d,%f,%c,\r\n"
         "LON,%d,%f,%c,\r\n",
         sentence->lat.deg, sentence->lat.min, sentence->lat.dir,
         sentence->lon.deg, sentence->lon.min, sentence->lon.dir);

  return b_TRUE;
}

static bool_t get_time(const char *cmd) {
  UNUSED(cmd);

  const nmea_sentence_t *sentence = nmea_get_last_sentence();

  printf("hr,min,sec\r\n"
         "%d,%d,%d\r\n",
         sentence->timestamp.hr, sentence->timestamp.min,
         sentence->timestamp.sec);

  return b_TRUE;
}

static bool_t raw_on(const char *cmd) {
  UNUSED(cmd);

  return nmea_set_raw(b_TRUE);
}

static bool_t raw_off(const char *cmd) {
  UNUSED(cmd);

  return nmea_set_raw(b_FALSE);
}

const cli_cmd_entry_t *cli_cmd_get_map(size_t *size) {
  *size = MAP_SIZE;
  return s_cli_cmd_map;
}