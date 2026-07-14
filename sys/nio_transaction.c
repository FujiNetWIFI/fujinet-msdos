#include "nio_transaction.h"
#include <string.h>

#ifdef __WATCOMC__
#define NIO_TX_MEMMOVE _fmemmove
#else
#define NIO_TX_MEMMOVE memmove
#endif

static void diag_reset(nio_transaction_diag_t *diag)
{
  if (!diag)
    return;
  diag->error = NIO_ERR_NONE;
  diag->status = NIO_STATUS_INTERNAL_ERROR;
  diag->rx_len = 0;
  diag->expected_len = 0;
  diag->lsr = 0;
}

static bool diag_fail(nio_transaction_diag_t *diag, uint8_t error,
                      uint16_t rx_len, uint16_t expected_len)
{
  if (diag) {
    diag->error = error;
    diag->rx_len = rx_len;
    diag->expected_len = expected_len;
  }
  return false;
}

static uint8_t nio_port_error(const nio_transaction_port_t *port,
                              uint8_t fallback)
{
  if (port->last_lsr && port->last_lsr(port->ctx))
    return NIO_ERR_UART;
  if (!port->last_slip_reason)
    return fallback;
  switch (port->last_slip_reason(port->ctx)) {
  case NIO_TRANSACTION_PORT_REASON_TIMEOUT:
    return NIO_ERR_TIMEOUT;
  case NIO_TRANSACTION_PORT_REASON_BUFFER_FULL:
    return NIO_ERR_BUFFER_FULL;
  case NIO_TRANSACTION_PORT_REASON_LINE_STATUS:
    return NIO_ERR_UART;
  default:
    return fallback;
  }
}

static bool nio_should_retry_error(uint8_t error)
{
  switch (error) {
  case NIO_ERR_UART:
  case NIO_ERR_TIMEOUT:
  case NIO_ERR_SHORT_FRAME:
  case NIO_ERR_LENGTH_MISMATCH:
  case NIO_ERR_CHECKSUM:
    return true;
  default:
    return false;
  }
}

static bool nio_transaction_once(const nio_transaction_port_t *port,
                                 const nio_transaction_buffers_t *buffers,
                                 uint8_t device, uint8_t command,
                                 const void NIO_PROTO_PTR *payload,
                                 uint16_t payload_length,
                                 void NIO_PROTO_PTR *reply,
                                 uint16_t reply_capacity,
                                 nio_response_t NIO_PROTO_PTR *response,
                                 uint16_t timeout,
                                 uint8_t max_stale_responses,
                                 nio_transaction_diag_t *diag)
{
  uint16_t rx_len;
  uint16_t rx_payload_len;
  nio_parsed_response_t parsed;
  uint8_t parse_error;
  uint8_t stale_count = 0;

  if (response) {
    response->status = NIO_STATUS_INTERNAL_ERROR;
    response->payload_length = 0;
  }

  port->flush_rx(port->ctx);
  port->putc(port->ctx, NIO_TRANSACTION_SLIP_END);
  port->putbuf_slip(port->ctx, buffers->tx_prefix,
                    sizeof(nio_frame_header_t));
  if (payload && payload_length)
    port->putbuf_slip(port->ctx, payload, payload_length);
  port->putc(port->ctx, NIO_TRANSACTION_SLIP_END);
  port->wait_tx_empty(port->ctx);

read_response:
  rx_len = port->getbuf_slip_dual(port->ctx,
                                  buffers->rx_header,
                                  sizeof(*buffers->rx_header),
                                  buffers->rx_payload,
                                  buffers->rx_payload_capacity,
                                  timeout);
  if (diag) {
    diag->rx_len = rx_len;
    diag->lsr = port->last_lsr ? port->last_lsr(port->ctx) : 0;
  }

  parse_error = nio_protocol_validate_response(buffers->rx_header,
                                               buffers->rx_payload,
                                               rx_len, device, command,
                                               reply_capacity, &parsed);
  if (diag)
    diag->expected_len = parsed.expected_len;
  if (parse_error == NIO_PROTO_ERR_DEVICE_COMMAND) {
    if (++stale_count <= max_stale_responses)
      goto read_response;
    return diag_fail(diag, parse_error, rx_len, parsed.expected_len);
  }
  if (parse_error != NIO_PROTO_OK)
    return diag_fail(diag, nio_port_error(port, parse_error),
                     rx_len, parsed.expected_len);

  if (diag) {
    diag->error = NIO_ERR_NONE;
    diag->status = parsed.status;
  }

  rx_payload_len = parsed.payload_length;
  if (reply && rx_payload_len)
    NIO_TX_MEMMOVE(reply, buffers->rx_payload + parsed.payload_offset,
                   rx_payload_len);

  if (response) {
    response->status = parsed.status;
    response->payload_length = rx_payload_len;
  }

  return true;
}

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
                          nio_transaction_diag_t *diag)
{
  nio_frame_header_t *tx;
  uint16_t checksum;
  uint16_t timeout;
  uint8_t attempt;
  uint8_t max_attempts;

  if (!port || !buffers || !buffers->tx_prefix || !buffers->rx_header ||
      !buffers->rx_payload ||
      buffers->tx_prefix_capacity < sizeof(nio_frame_header_t) ||
      !port->flush_rx || !port->putc || !port->putbuf_slip ||
      !port->wait_tx_empty || !port->getbuf_slip_dual)
    return diag_fail(diag, NIO_ERR_PAYLOAD, 0, 0);

  tx = (nio_frame_header_t *) buffers->tx_prefix;
  tx->device = device;
  tx->command = command;
  tx->length = (uint16_t) (sizeof(*tx) + payload_length);
  tx->checksum = 0;
  tx->fields = NIO_PROTO_FIELD_NONE;

  checksum = nio_protocol_checksum(tx, sizeof(*tx), 0);
  if (payload)
    checksum = nio_protocol_checksum(payload, payload_length, checksum);
  tx->checksum = (uint8_t) checksum;

  timeout = (device == NIO_DEVICEID_NETWORK) ? network_timeout_ms
                                             : default_timeout_ms;
  max_attempts = (uint8_t) (transport_retries + 1);
  if (max_attempts == 0)
    max_attempts = 1;

  for (attempt = 0; attempt < max_attempts; attempt++) {
    diag_reset(diag);

    if (nio_transaction_once(port, buffers, device, command,
                             payload, payload_length, reply, reply_capacity,
                             response, timeout, max_stale_responses, diag))
      return true;

    if (!diag || !nio_should_retry_error(diag->error))
      return false;
  }

  return false;
}
