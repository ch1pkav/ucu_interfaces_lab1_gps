#ifndef _LAB1_FORWARD_H
#define _LAB1_FORWARD_H

#include "usbd_cdc_if.h"

typedef enum {
  b_FALSE = 0,
  b_TRUE = 1,
} bool_t;

#define RAW_PRINT(buf, size) CDC_Transmit_FS((uint8_t *)buf, size);

#define RINGBUF_ISR_COPY(buf_name, max_size) \
  if (size + buf_name ## _buf_offset < max_size) { \
    memcpy((void *)&buf_name ## _buf[buf_name ## _buf_offset], buf, size); \
  } else { \
    size_t to_end = max_size - buf_name ## _buf_offset; \
    memcpy((void *)&buf_name ## _buf[buf_name ## _buf_offset], buf, to_end); \
    memcpy((void *)&buf_name ## _buf, \
           &buf[max_size - buf_name ## _buf_offset], \
           size - to_end); \
  } \
  buf_name ## _buf_offset = \
      (buf_name ## _buf_offset + size) % max_size; \
  buf_name ## _last_chunk_size = size; \
  buf_name ## _data_ready = b_TRUE


#define RINGBUF_DECLARE(buf_name, ptr) \
  static volatile uint8_t* buf_name##_buf = ptr; \
  static volatile size_t buf_name##_buf_offset = 0; \
  static volatile size_t buf_name##_last_chunk_size = 0; \
  static volatile bool_t buf_name##_data_ready = b_FALSE


#endif //_LAB1_FORWARD_H