/**
 * FujiNet NIO low-level protocol helpers.
 */

#include "nio.h"
#include "nio_disk_protocol.h"
#include "nio_protocol.h"
#include "nio_transaction.h"
#include "portio.h"
#include <string.h>

#define NIO_TIMEOUT_SLOW (15 * 1000)
#define NIO_MAX_RX       9216
#define NIO_MAX_TX_PREFIX 32
#define NIO_MAX_STALE_RESPONSES 2

static uint8_t tx_prefix[NIO_MAX_TX_PREFIX];
static uint8_t rx_payload[NIO_MAX_RX];
static nio_frame_header_t rx_header;
uint8_t nio_last_error;
uint8_t nio_last_status;
uint16_t nio_last_rx_len;
uint16_t nio_last_expected_len;
uint8_t nio_last_lsr;
uint16_t nio_network_timeout_ms = NIO_TIMEOUT_SLOW;
uint8_t nio_transport_retries = 2;

static bool nio_fail(uint8_t error, uint16_t rx_len, uint16_t expected_len)
{
  nio_last_error = error;
  nio_last_rx_len = rx_len;
  nio_last_expected_len = expected_len;
  return false;
}

static void nio_port_flush_rx(void *ctx)
{
  (void) ctx;
  port_flush_rx();
}

static int nio_port_putc(void *ctx, uint8_t c)
{
  (void) ctx;
  return port_putc(c);
}

static uint16_t nio_port_putbuf_slip(void *ctx, const void far *buf,
                                     uint16_t len)
{
  (void) ctx;
  return port_putbuf_slip(buf, len);
}

static void nio_port_wait_tx_empty(void *ctx)
{
  (void) ctx;
  port_wait_tx_empty();
}

static uint16_t nio_port_getbuf_slip_dual(void *ctx,
                                          void *hdr_buf, uint16_t hdr_len,
                                          void far *data_buf,
                                          uint16_t data_len,
                                          uint16_t timeout)
{
  (void) ctx;
  return port_getbuf_slip_dual(hdr_buf, hdr_len, data_buf, data_len, timeout);
}

static uint8_t nio_port_last_slip_reason(void *ctx)
{
  (void) ctx;
  return port_slip_last_reason;
}

static uint8_t nio_port_last_lsr(void *ctx)
{
  (void) ctx;
  return port_slip_last_lsr;
}

static nio_transaction_port_t nio_port;
static nio_transaction_buffers_t nio_buffers;
static nio_transaction_diag_t nio_diag;
static uint8_t nio_transaction_bound;

static void nio_bind_transaction(void)
{
  if (nio_transaction_bound)
    return;

  nio_port.ctx = 0;
  nio_port.flush_rx = nio_port_flush_rx;
  nio_port.putc = nio_port_putc;
  nio_port.putbuf_slip = nio_port_putbuf_slip;
  nio_port.wait_tx_empty = nio_port_wait_tx_empty;
  nio_port.getbuf_slip_dual = nio_port_getbuf_slip_dual;
  nio_port.last_slip_reason = nio_port_last_slip_reason;
  nio_port.last_lsr = nio_port_last_lsr;

  nio_buffers.tx_prefix = tx_prefix;
  nio_buffers.tx_prefix_capacity = sizeof(tx_prefix);
  nio_buffers.rx_header = &rx_header;
  nio_buffers.rx_payload = rx_payload;
  nio_buffers.rx_payload_capacity = sizeof(rx_payload);

  nio_transaction_bound = 1;
}

bool nio_status_ok(uint8_t status)
{
  return status == NIO_STATUS_OK;
}

bool nio_call(uint8_t device, uint8_t command,
              const void far *payload, uint16_t payload_length,
              void far *reply, uint16_t reply_capacity,
              nio_response_t far *response)
{
  bool ok;

  nio_bind_transaction();
  ok = nio_transaction_call(&nio_port, &nio_buffers, device, command,
                            payload, payload_length, reply, reply_capacity,
                            response, nio_transport_retries,
                            nio_network_timeout_ms, NIO_TIMEOUT_SLOW,
                            NIO_MAX_STALE_RESPONSES, &nio_diag);
  nio_last_error = nio_diag.error;
  nio_last_status = nio_diag.status;
  nio_last_rx_len = nio_diag.rx_len;
  nio_last_expected_len = nio_diag.expected_len;
  nio_last_lsr = nio_diag.lsr;
  return ok;
}

bool nio_disk_info(uint8_t slot, nio_disk_info_t far *info)
{
  uint8_t req[2];
  uint8_t resp[16];
  nio_response_t nr;

  nio_disk_build_slot_request(slot, req, sizeof(req));

  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_INFO,
                req, NIO_DISK_SLOT_REQUEST_SIZE, resp, sizeof(resp), &nr))
    return false;
  if (!nio_status_ok(nr.status) ||
      !nio_disk_parse_info_response(resp, nr.payload_length, info))
    return nio_fail(nio_status_ok(nr.status) ? NIO_ERR_PAYLOAD : NIO_ERR_STATUS,
                    nio_last_rx_len, nio_last_expected_len);
  return true;
}

bool nio_disk_clear_changed(uint8_t slot)
{
  uint8_t req[2];
  uint8_t resp[8];
  nio_response_t nr;

  nio_disk_build_slot_request(slot, req, sizeof(req));

  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_CLEAR_CHANGED,
                req, NIO_DISK_SLOT_REQUEST_SIZE, resp, sizeof(resp), &nr))
    return false;
  if (!nio_status_ok(nr.status) ||
      !nio_disk_validate_response(resp, nr.payload_length,
                                  NIO_DISK_CLEAR_CHANGED_RESPONSE_MIN_SIZE))
    return nio_fail(nio_status_ok(nr.status) ? NIO_ERR_PAYLOAD : NIO_ERR_STATUS,
                    nio_last_rx_len, nio_last_expected_len);
  return true;
}

bool nio_disk_read_sector(uint8_t slot, uint32_t lba,
                          void far *buffer, uint16_t buffer_length,
                          uint16_t far *bytes_read)
{
  uint8_t req[8];
  nio_response_t nr;
  uint16_t data_len;

  nio_disk_build_read_sector_request(slot, lba, buffer_length, req, sizeof(req));

  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_READ_SECTOR,
                req, NIO_DISK_READ_SECTOR_REQUEST_SIZE, rx_payload,
                sizeof(rx_payload), &nr))
    return false;
  if (!nio_status_ok(nr.status) ||
      !nio_disk_parse_read_sector_response(rx_payload, nr.payload_length,
                                           buffer_length, &data_len))
    return nio_fail(nio_status_ok(nr.status) ? NIO_ERR_PAYLOAD : NIO_ERR_STATUS,
                    nio_last_rx_len, nio_last_expected_len);

  if (buffer && data_len)
    _fmemcpy(buffer, rx_payload + 11, data_len);
  if (bytes_read)
    *bytes_read = data_len;
  return true;
}

bool nio_disk_read_sectors(uint8_t slot, uint32_t lba, uint16_t count,
                           void far *buffer, uint16_t buffer_length,
                           uint16_t far *bytes_read)
{
  uint8_t req[10];
  nio_response_t nr;
  uint16_t data_len;

  nio_disk_build_read_sectors_request(slot, lba, count, buffer_length,
                                      req, sizeof(req));

  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_READ_SECTORS,
                req, NIO_DISK_READ_SECTORS_REQUEST_SIZE, rx_payload,
                sizeof(rx_payload), &nr))
    return false;
  if (!nio_status_ok(nr.status) ||
      !nio_disk_parse_read_sectors_response(rx_payload, nr.payload_length,
                                            buffer_length, &data_len))
    return nio_fail(nio_status_ok(nr.status) ? NIO_ERR_PAYLOAD : NIO_ERR_STATUS,
                    nio_last_rx_len, nio_last_expected_len);

  if (buffer && data_len)
    _fmemcpy(buffer, rx_payload + 13, data_len);
  if (bytes_read)
    *bytes_read = data_len;
  return true;
}

bool nio_disk_write_sector(uint8_t slot, uint32_t lba,
                           const void far *buffer, uint16_t buffer_length,
                           uint16_t far *bytes_written)
{
  uint8_t req_prefix[8];
  uint8_t resp[16];
  nio_response_t nr;

  req_prefix[0] = NIO_DISK_VERSION;
  req_prefix[1] = slot;
  nio_disk_put_u32le(&req_prefix[2], lba);
  nio_disk_put_u16le(&req_prefix[6], buffer_length);

  /* Build and send this request manually so sector data does not need to be
     copied into a second 16-bit DOS buffer. */
  {
    nio_frame_header_t *tx = (nio_frame_header_t *) tx_prefix;
    uint16_t checksum;
    uint16_t rx_len;
    uint16_t rx_payload_len;
    nio_parsed_response_t parsed;
    uint8_t parse_error;

    tx->device = NIO_DEVICEID_DISK;
    tx->command = NIO_DISK_CMD_WRITE_SECTOR;
    tx->length = sizeof(*tx) + sizeof(req_prefix) + buffer_length;
    tx->checksum = 0;
    tx->fields = NIO_PROTO_FIELD_NONE;

    checksum = nio_protocol_checksum(tx, sizeof(*tx), 0);
    checksum = nio_protocol_checksum(req_prefix, sizeof(req_prefix), checksum);
    checksum = nio_protocol_checksum(buffer, buffer_length, checksum);
    tx->checksum = (uint8_t) checksum;

    port_flush_rx();
    port_putc(NIO_TRANSACTION_SLIP_END);
    port_putbuf_slip(tx_prefix, sizeof(*tx));
    port_putbuf_slip(req_prefix, sizeof(req_prefix));
    if (buffer && buffer_length)
      port_putbuf_slip(buffer, buffer_length);
    port_putc(NIO_TRANSACTION_SLIP_END);
    port_wait_tx_empty();

    rx_len = port_getbuf_slip_dual(&rx_header, sizeof(rx_header),
                                   resp, sizeof(resp), NIO_TIMEOUT_SLOW);
    parse_error = nio_protocol_validate_response(&rx_header, resp, rx_len,
                                                 NIO_DEVICEID_DISK,
                                                 NIO_DISK_CMD_WRITE_SECTOR,
                                                 sizeof(resp), &parsed);
    if (parse_error != NIO_PROTO_OK)
      return false;

    nr.status = parsed.status;
    rx_payload_len = parsed.payload_length;
    nr.payload_length = rx_payload_len;
    if (!nio_status_ok(nr.status) || nr.payload_length < 11 || resp[1] != NIO_DISK_VERSION)
      return false;
  if (bytes_written)
      *bytes_written = nio_disk_get_u16le(&resp[10]);
  }

  return true;
}

bool nio_disk_write_sectors(uint8_t slot, uint32_t lba, uint16_t count,
                            const void far *buffer, uint16_t buffer_length,
                            uint16_t far *bytes_written)
{
  uint8_t req_prefix[10];
  uint8_t resp[16];
  nio_response_t nr;

  req_prefix[0] = NIO_DISK_VERSION;
  req_prefix[1] = slot;
  nio_disk_put_u32le(&req_prefix[2], lba);
  nio_disk_put_u16le(&req_prefix[6], count);
  nio_disk_put_u16le(&req_prefix[8], buffer_length);

  {
    nio_frame_header_t *tx = (nio_frame_header_t *) tx_prefix;
    uint16_t checksum;
    uint16_t rx_len;
    uint16_t rx_payload_len;
    nio_parsed_response_t parsed;
    uint8_t parse_error;

    tx->device = NIO_DEVICEID_DISK;
    tx->command = NIO_DISK_CMD_WRITE_SECTORS;
    tx->length = sizeof(*tx) + sizeof(req_prefix) + buffer_length;
    tx->checksum = 0;
    tx->fields = NIO_PROTO_FIELD_NONE;

    checksum = nio_protocol_checksum(tx, sizeof(*tx), 0);
    checksum = nio_protocol_checksum(req_prefix, sizeof(req_prefix), checksum);
    checksum = nio_protocol_checksum(buffer, buffer_length, checksum);
    tx->checksum = (uint8_t) checksum;

    port_flush_rx();
    port_putc(NIO_TRANSACTION_SLIP_END);
    port_putbuf_slip(tx_prefix, sizeof(*tx));
    port_putbuf_slip(req_prefix, sizeof(req_prefix));
    if (buffer && buffer_length)
      port_putbuf_slip(buffer, buffer_length);
    port_putc(NIO_TRANSACTION_SLIP_END);
    port_wait_tx_empty();

    rx_len = port_getbuf_slip_dual(&rx_header, sizeof(rx_header),
                                   resp, sizeof(resp), NIO_TIMEOUT_SLOW);
    parse_error = nio_protocol_validate_response(&rx_header, resp, rx_len,
                                                 NIO_DEVICEID_DISK,
                                                 NIO_DISK_CMD_WRITE_SECTORS,
                                                 sizeof(resp), &parsed);
    if (parse_error != NIO_PROTO_OK)
      return false;

    nr.status = parsed.status;
    rx_payload_len = parsed.payload_length;
    nr.payload_length = rx_payload_len;
    if (!nio_status_ok(nr.status) || nr.payload_length < 13 || resp[1] != NIO_DISK_VERSION)
      return false;
  if (bytes_written)
      *bytes_written = nio_disk_get_u16le(&resp[12]);
  }

  return true;
}
