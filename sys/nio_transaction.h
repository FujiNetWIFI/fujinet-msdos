#ifndef FUJINET_MSDOS_NIO_TRANSACTION_H
#define FUJINET_MSDOS_NIO_TRANSACTION_H

#include "nio_protocol.h"
#include "../include/nio.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  NIO_TRANSACTION_SLIP_END = 0xC0
};

enum {
  NIO_TRANSACTION_PORT_REASON_NONE = 0,
  NIO_TRANSACTION_PORT_REASON_TIMEOUT = 1,
  NIO_TRANSACTION_PORT_REASON_BUFFER_FULL = 2,
  NIO_TRANSACTION_PORT_REASON_LINE_STATUS = 3
};

typedef struct {
  void *ctx;
  void (*flush_rx)(void *ctx);
  int (*putc)(void *ctx, uint8_t c);
  uint16_t (*putbuf_slip)(void *ctx, const void NIO_PROTO_PTR *buf,
                          uint16_t len);
  void (*wait_tx_empty)(void *ctx);
  uint16_t (*getbuf_slip_dual)(void *ctx,
                               void *hdr_buf, uint16_t hdr_len,
                               void NIO_PROTO_PTR *data_buf,
                               uint16_t data_len, uint16_t timeout);
  uint8_t (*last_slip_reason)(void *ctx);
  uint8_t (*last_lsr)(void *ctx);
} nio_transaction_port_t;

typedef struct {
  uint8_t NIO_PROTO_PTR *tx_prefix;
  uint16_t tx_prefix_capacity;
  nio_frame_header_t *rx_header;
  uint8_t NIO_PROTO_PTR *rx_payload;
  uint16_t rx_payload_capacity;
} nio_transaction_buffers_t;

typedef struct {
  uint8_t error;
  uint8_t status;
  uint16_t rx_len;
  uint16_t expected_len;
  uint8_t lsr;
} nio_transaction_diag_t;

bool nio_transaction_call(const nio_transaction_port_t *port,
                          const nio_transaction_buffers_t *buffers,
                          uint8_t device, uint8_t command,
                          const void NIO_PROTO_PTR *payload,
                          uint16_t payload_length,
                          void NIO_PROTO_PTR *reply,
                          uint16_t reply_capacity,
                          nio_response_t NIO_PROTO_PTR *response,
                          uint8_t transport_retries,
                          uint16_t network_timeout_ms,
                          uint16_t default_timeout_ms,
                          uint8_t max_stale_responses,
                          nio_transaction_diag_t *diag);

#ifdef __cplusplus
}
#endif

#endif /* FUJINET_MSDOS_NIO_TRANSACTION_H */
