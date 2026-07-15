#include "commands.h"
#include "fujinet.h"
#include "sys_hdr.h"
#include "nio.h"
#include "print.h"
#include "ioctl.h"
#include <string.h>
#include <dos.h>
#include <stdlib.h>

#undef DEBUG

#define SECTOR_SIZE     512
#define MAX_BATCH_SECTORS 16
#define DEFAULT_BATCH_SECTORS 16
#define DEFAULT_IO_RETRIES 2
#define DEFAULT_NIO_RETRIES 2
#define DEFAULT_NETWORK_TIMEOUT_MS (15 * 1000)
/* DOS-side metadata/data cache. 40 sectors preserves the observed directory
   metadata working set better than 24 while still freeing 8 KiB compared with
   the earlier 56-sector cache. */
#define CACHE_SECTORS 40

extern void End_code(void);

DOS_BPB fn_bpb_table[FN_MAX_DEV];
DOS_BPB *fn_bpb_pointers[FN_MAX_DEV + 1]; // leave room for the NULL terminator

static uint8_t nio_unit_slot[FN_MAX_DEV] = {0, 1, 2, 3, 4, 5, 6, 7};
static char nio_current_uri[FUJI_IOCTL_MAX_URI + 1];
static char nio_display_path[FUJI_IOCTL_MAX_PATH + 1];
static uint16_t nio_current_uri_len;
static uint16_t nio_display_path_len;
static uint8_t cache_valid[CACHE_SECTORS];
static uint8_t cache_unit[CACHE_SECTORS];
static uint32_t cache_lba[CACHE_SECTORS];
static uint8_t cache_data[CACHE_SECTORS][SECTOR_SIZE];
static uint8_t cache_next;
static uint8_t info_cache_valid[FN_MAX_DEV];
static nio_disk_info_t info_cache[FN_MAX_DEV];
static uint8_t media_changed[FN_MAX_DEV];
static uint8_t nio_batch_sectors = DEFAULT_BATCH_SECTORS;
static uint8_t nio_io_retries = DEFAULT_IO_RETRIES;
static uint8_t nio_debug_io;
static uint8_t nio_auto_downshift = 1;

#ifdef OBSOLETE
static cmdFrame_t cmd; // FIXME - make this shared with init.c?
#endif /* OBSOLETE */

static uint8_t unit_to_slot(uint8_t unit)
{
  if (unit >= FN_MAX_DEV)
    return 0xFF;
  return nio_unit_slot[unit];
}

static uint8_t parse_sector_count(const char *name, uint8_t default_value)
{
  const char *value = getenv(name);
  unsigned count;

  if (!value)
    return default_value;

  count = (unsigned) atoi(value);
  if (count < 1)
    count = 1;
  if (count > MAX_BATCH_SECTORS)
    count = MAX_BATCH_SECTORS;
  return (uint8_t) count;
}

static uint8_t parse_retry_count(const char *name, uint8_t default_value)
{
  const char *value = getenv(name);
  unsigned count;

  if (!value)
    return default_value;

  count = (unsigned) atoi(value);
  if (count > MAX_BATCH_SECTORS)
    count = MAX_BATCH_SECTORS;
  return (uint8_t) count;
}

static uint8_t parse_flag(const char *name, uint8_t default_value)
{
  const char *value = getenv(name);

  if (!value)
    return default_value;
  if (value[0] == '0' || value[0] == 'n' || value[0] == 'N')
    return 0;
  return 1;
}

static uint16_t parse_timeout_ms(const char *name, uint16_t default_value)
{
  const char *value = getenv(name);
  unsigned timeout;

  if (!value)
    return default_value;

  timeout = (unsigned) atoi(value);
  if (timeout < 250)
    timeout = 250;
  if (timeout > 60000)
    timeout = 60000;
  return (uint16_t) timeout;
}

void nio_driver_config_init(void)
{
  nio_batch_sectors = parse_sector_count("FUJI_BATCH_SECTORS", DEFAULT_BATCH_SECTORS);
  nio_io_retries = parse_retry_count("FUJI_IO_RETRIES", DEFAULT_IO_RETRIES);
  nio_transport_retries = parse_retry_count("FUJI_NIO_RETRIES", DEFAULT_NIO_RETRIES);
  nio_debug_io = parse_flag("FUJI_DEBUG_IO", 0);
  nio_auto_downshift = parse_flag("FUJI_AUTO_DOWNSHIFT", 1);
  nio_network_timeout_ms = parse_timeout_ms("FUJI_NET_TIMEOUT_MS", DEFAULT_NETWORK_TIMEOUT_MS);

  consolef("NIO batch/retries: %i/%i tx=%i\n",
           nio_batch_sectors, nio_io_retries, nio_transport_retries);
  consolef("NIO network timeout: %i ms\n", nio_network_timeout_ms);
}

static uint8_t unit_to_diskservice_slot(uint8_t unit)
{
  uint8_t slot = unit_to_slot(unit);
  if (slot == 0xFF)
    return 0xFF;
  return slot + 1;
}

static void fill_query(fuji_ioctl_query far *query, uint8_t unit, uint16_t query_len)
{
  _fmemcpy(query->signature, FUJI_IOCTL_SIGNATURE, 4);
  query->unit = unit;
  if (query_len >= sizeof(*query)) {
    query->version = FUJI_IOCTL_VERSION;
    query->max_units = FN_MAX_DEV;
  }
}

uint16_t nio_handle_control_call(fuji_ioctl_nio_call far *call, uint8_t unit)
{
  nio_response_t response;
  uint16_t uri_len;
  uint16_t path_len;
  uint16_t offset;

  if (call->request_len > FUJI_IOCTL_MAX_DATA ||
      call->response_len > FUJI_IOCTL_MAX_DATA)
    return ERROR_BIT | BAD_REQ_LEN;

  if (call->device == 0x00 && call->nio_command == FUJI_IOCTL_SET_STATE) {
    if (call->request_len < 4)
      return ERROR_BIT | BAD_REQ_LEN;
    uri_len = (uint16_t) call->data[0] | ((uint16_t) call->data[1] << 8);
    path_len = (uint16_t) call->data[2] | ((uint16_t) call->data[3] << 8);
    offset = 4;
    if (uri_len > FUJI_IOCTL_MAX_URI || path_len > FUJI_IOCTL_MAX_PATH ||
        call->request_len < (uint16_t) (offset + uri_len + path_len))
      return ERROR_BIT | BAD_REQ_LEN;

    nio_current_uri_len = uri_len;
    nio_display_path_len = path_len;
    _fmemset(nio_current_uri, 0, sizeof(nio_current_uri));
    _fmemset(nio_display_path, 0, sizeof(nio_display_path));
    _fmemcpy(nio_current_uri, &call->data[offset], nio_current_uri_len);
    offset += uri_len;
    _fmemcpy(nio_display_path, &call->data[offset], nio_display_path_len);

    call->nio_status = NIO_STATUS_OK;
    call->response_len = 0;
    fill_query((fuji_ioctl_query far *) call, unit, sizeof(*call));
    return OP_COMPLETE;
  }

  if (!nio_call(call->device, call->nio_command,
                call->data, call->request_len,
                call->data, call->response_len,
                &response)) {
    if (nio_debug_io)
      consolef("FN nio fail dev=%x cmd=%x err=%i st=%i rx=%i exp=%i\n",
               call->device, call->nio_command, nio_last_error,
               nio_last_status, nio_last_rx_len, nio_last_expected_len);
    call->nio_status = NIO_STATUS_IO_ERROR;
    call->response_len = 0;
    call->diag_error = nio_last_error;
    call->diag_status = nio_last_status;
    call->diag_rx_len = nio_last_rx_len;
    call->diag_expected_len = nio_last_expected_len;
    call->diag_lsr = nio_last_lsr;
    fill_query((fuji_ioctl_query far *) call, unit, sizeof(*call));
    return OP_COMPLETE;
  }

  call->nio_status = response.status;
  call->response_len = response.payload_length;
  call->diag_error = nio_last_error;
  call->diag_status = nio_last_status;
  call->diag_rx_len = nio_last_rx_len;
  call->diag_expected_len = nio_last_expected_len;
  call->diag_lsr = nio_last_lsr;
  fill_query((fuji_ioctl_query far *) call, unit, sizeof(*call));
  return OP_COMPLETE;
}

static int cache_find(uint8_t unit, uint32_t lba)
{
  uint8_t i;
  for (i = 0; i < CACHE_SECTORS; i++) {
    if (cache_valid[i] && cache_unit[i] == unit && cache_lba[i] == lba)
      return i;
  }
  return -1;
}

static void cache_store(uint8_t unit, uint32_t lba, const uint8_t far *data)
{
  uint8_t idx = cache_next;
  cache_valid[idx] = 1;
  cache_unit[idx] = unit;
  cache_lba[idx] = lba;
  _fmemcpy(cache_data[idx], data, SECTOR_SIZE);
  cache_next = (uint8_t) ((cache_next + 1) % CACHE_SECTORS);
}

static void cache_update_or_invalidate(uint8_t unit, uint32_t lba, const uint8_t far *data)
{
  int idx = cache_find(unit, lba);
  if (idx >= 0) {
    if (data)
      _fmemcpy(cache_data[idx], data, SECTOR_SIZE);
    else
      cache_valid[idx] = 0;
  }
}

static void cache_invalidate_unit(uint8_t unit)
{
  uint8_t i;
  for (i = 0; i < CACHE_SECTORS; i++) {
    if (cache_valid[i] && cache_unit[i] == unit)
      cache_valid[i] = 0;
  }
  if (unit < FN_MAX_DEV)
    info_cache_valid[unit] = 0;
}

static bool disk_info_cached(uint8_t unit, nio_disk_info_t far *info)
{
  if (unit >= FN_MAX_DEV)
    return false;
  if (info_cache_valid[unit]) {
    _fmemcpy(info, &info_cache[unit], sizeof(*info));
    return true;
  }
  if (!nio_disk_info(unit_to_diskservice_slot(unit), info))
    return false;
  _fmemcpy(&info_cache[unit], info, sizeof(*info));
  info_cache_valid[unit] = 1;
  return true;
}

static uint16_t handle_ioctl_buffer(SYSREQ far *req)
{
  uint8_t far *buffer = req->io.buffer_ptr;
  uint8_t command;

  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid IOCTL unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

  if (req->io.count < FUJI_IOCTL_QUERY_BASE_SIZE)
    return ERROR_BIT | UNKNOWN_CMD;

  command = buffer[0];

  if (req->io.count < 5)
    return ERROR_BIT | UNKNOWN_CMD;

  if (command == FUJI_IOCTL_QUERY) {
    fill_query((fuji_ioctl_query far *) buffer, req->unit, req->io.count);
    return OP_COMPLETE;
  }

  switch (command) {
  case FUJI_IOCTL_GET_STATE:
  {
    fuji_ioctl_state far *state = (fuji_ioctl_state far *) buffer;
    if (req->io.count < sizeof(*state))
      return ERROR_BIT | UNKNOWN_CMD;

    fill_query((fuji_ioctl_query far *) state, req->unit, sizeof(*state));
    state->current_uri_len = nio_current_uri_len;
    state->display_path_len = nio_display_path_len;
    _fmemcpy(state->current_uri, nio_current_uri, sizeof(nio_current_uri));
    _fmemcpy(state->display_path, nio_display_path, sizeof(nio_display_path));
    return OP_COMPLETE;
  }

  case FUJI_IOCTL_SET_STATE:
  {
    fuji_ioctl_state far *state = (fuji_ioctl_state far *) buffer;
    if (req->io.count < sizeof(*state))
      return ERROR_BIT | UNKNOWN_CMD;
    if (state->current_uri_len > FUJI_IOCTL_MAX_URI ||
        state->display_path_len > FUJI_IOCTL_MAX_PATH)
      return ERROR_BIT | BAD_REQ_LEN;

    nio_current_uri_len = state->current_uri_len;
    nio_display_path_len = state->display_path_len;
    _fmemset(nio_current_uri, 0, sizeof(nio_current_uri));
    _fmemset(nio_display_path, 0, sizeof(nio_display_path));
    _fmemcpy(nio_current_uri, state->current_uri, nio_current_uri_len);
    _fmemcpy(nio_display_path, state->display_path, nio_display_path_len);
    fill_query((fuji_ioctl_query far *) state, req->unit, sizeof(*state));
    return OP_COMPLETE;
  }

  case FUJI_IOCTL_GET_UNIT_MAP:
  {
    fuji_ioctl_unit_map far *map = (fuji_ioctl_unit_map far *) buffer;
    uint8_t requested_unit;
    if (req->io.count < sizeof(*map))
      return ERROR_BIT | UNKNOWN_CMD;
    if (map->unit >= FN_MAX_DEV)
      return ERROR_BIT | UNKNOWN_UNIT;

    requested_unit = map->unit;
    fill_query((fuji_ioctl_query far *) map, req->unit, sizeof(*map));
    map->unit = requested_unit;
    map->slot = nio_unit_slot[requested_unit];
    return OP_COMPLETE;
  }

  case FUJI_IOCTL_SET_UNIT_MAP:
  {
    fuji_ioctl_unit_map far *map = (fuji_ioctl_unit_map far *) buffer;
    if (req->io.count < sizeof(*map))
      return ERROR_BIT | UNKNOWN_CMD;
    if (map->unit >= FN_MAX_DEV)
      return ERROR_BIT | UNKNOWN_UNIT;
    if (map->slot >= FN_MAX_DEV)
      return ERROR_BIT | BAD_REQ_LEN;

    cache_invalidate_unit(map->unit);
    nio_unit_slot[map->unit] = map->slot;
    media_changed[map->unit] = 1;
    fill_query((fuji_ioctl_query far *) map, req->unit, sizeof(*map));
    return OP_COMPLETE;
  }

  case FUJI_IOCTL_NIO_CALL:
  {
    fuji_ioctl_nio_call far *call = (fuji_ioctl_nio_call far *) buffer;

    if (req->io.count < sizeof(*call))
      return ERROR_BIT | UNKNOWN_CMD;
    return nio_handle_control_call(call, req->unit);
  }

  default:
    return ERROR_BIT | UNKNOWN_CMD;
  }
}

uint16_t Media_check_cmd(SYSREQ far *req)
{
  nio_disk_info_t info;

  int i=0;

  // Avoid race condition that only happens on PCjr systems
  // I do not know why this works. -Thom
  for (i=0;i<8192;i++);

  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid Media Check unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

  if (media_changed[req->unit]) {
    media_changed[req->unit] = 0;
    info_cache_valid[req->unit] = 0;
    req->media.return_info = -1;
    return OP_COMPLETE;
  }

  if (!disk_info_cached(req->unit, &info))
    return ERROR_BIT | NOT_READY;

  if (!(info.flags & NIO_DISK_INFO_INSERTED))
    req->media.return_info = 0;
  else if (info.flags & NIO_DISK_INFO_CHANGED)
    req->media.return_info = -1;
  else
    req->media.return_info = 1;

  return OP_COMPLETE;
}

uint16_t Build_bpb_cmd(SYSREQ far *req)
{
  uint8_t far *buf;
  uint16_t bytes_read;
  int cached;
  nio_disk_info_t info;


  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid BPB unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

  // DOS gave us a buffer to use
  buf = req->bpb.buffer_ptr;

  if (disk_info_cached(req->unit, &info) &&
      !(info.flags & NIO_DISK_INFO_INSERTED)) {
    req->bpb.table = MK_FP(getCS(), fn_bpb_pointers[req->unit]);
    return OP_COMPLETE;
  }

  cached = cache_find(req->unit, 0);
  if (cached >= 0) {
    _fmemcpy(buf, cache_data[cached], SECTOR_SIZE);
  } else {
    if (!nio_disk_read_sector(unit_to_diskservice_slot(req->unit), 0, buf, SECTOR_SIZE, &bytes_read)
        || bytes_read < SECTOR_SIZE) {
      consolef("FujiNet NIO BPB read fail\n");
      return ERROR_BIT | READ_FAULT;
    }
    cache_store(req->unit, 0, buf);
  }

  _fmemcpy(fn_bpb_pointers[req->unit], &buf[0x0b], sizeof(DOS_BPB));
  if (nio_disk_clear_changed(unit_to_diskservice_slot(req->unit)))
    info_cache_valid[req->unit] = 0;

#if 0
  consolef("BPB for %i\n", req->unit);
  dumpHex((uint8_t far *) fn_bpb_pointers[req->unit], sizeof(DOS_BPB));
#endif

  req->bpb.table = MK_FP(getCS(), fn_bpb_pointers[req->unit]);

  return OP_COMPLETE;
}

uint16_t Ioctl_input_cmd(SYSREQ far *req)
{
#if 0
  consolef("IOCTL INPUT CALLED\n");
  consolef("UNIT: %d\n", req->unit);
  consolef("SIZE: %d\n", req->io.count);
  consolef("BUFFER: %08lx\n", req->io.buffer_ptr);
#endif

  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid Input unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

  return handle_ioctl_buffer(req);
}

uint16_t Input_cmd(SYSREQ far *req)
{
  uint16_t done = 0;
  uint32_t sector, sector_max;
  uint8_t far *buf = req->io.buffer_ptr;
  uint16_t bytes_read = 0;


  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid Input unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

#if 0
  dumpHex((uint8_t far *) req, req->length);
  consolef("SECTOR: %i 0x%04x 0x%08lx\n", req->length,
           req->io.start_sector, (uint32_t) req->io.start_sector_32);
#endif

  if (req->length > 22)
    sector = req->io.start_sector_32;
  else
    sector = req->io.start_sector;

  if (fn_bpb_table[req->unit].num_sectors)
    sector_max = fn_bpb_table[req->unit].num_sectors;
  else
    sector_max = fn_bpb_table[req->unit].num_sectors_32;

#if 0
  consolef("SECTOR: %i 0x%08lx %i SM: %i\n", req->length, sector, req->io.count, sector_max);
#endif

  while (done < req->io.count) {
    uint16_t remaining = req->io.count - done;
    uint16_t batch = remaining > nio_batch_sectors ? nio_batch_sectors : remaining;
    uint16_t fetch;
    int cached;
    uint16_t fill;
    uint8_t attempt;
    uint8_t read_ok = 0;
    uint8_t downshift = 0;
    bytes_read = 0;

    if (sector >= sector_max) {
      consolef("FN Invalid sector read %li on %i\n", sector, req->unit);
      return ERROR_BIT | NOT_FOUND;
    }

    cached = cache_find(req->unit, sector);
    if (cached >= 0) {
      _fmemcpy(&buf[done * SECTOR_SIZE], cache_data[cached], SECTOR_SIZE);
      done++;
      sector++;
      continue;
    }

    if ((uint32_t) batch > (sector_max - sector))
      batch = (uint16_t) (sector_max - sector);

    fetch = batch;

    for (attempt = 0; attempt <= nio_io_retries; attempt++) {
      bytes_read = 0;
      if (nio_disk_read_sectors(unit_to_diskservice_slot(req->unit), sector, fetch,
                                &buf[done * SECTOR_SIZE],
                                (uint16_t) (fetch * SECTOR_SIZE),
                                &bytes_read) &&
          bytes_read >= (uint16_t) (fetch * SECTOR_SIZE)) {
        read_ok = 1;
        break;
      }

      if (nio_auto_downshift && fetch > 1 &&
          (nio_last_error == NIO_ERR_SHORT_FRAME ||
           nio_last_error == NIO_ERR_LENGTH_MISMATCH ||
           nio_last_error == NIO_ERR_CHECKSUM)) {
        uint8_t new_limit = (uint8_t) (fetch >> 1);
        if (new_limit < 1)
          new_limit = 1;
        if (nio_batch_sectors > new_limit)
          nio_batch_sectors = new_limit;
        consolef("FN read downshift n=%i err=%i rx=%i exp=%i\n",
                 new_limit, nio_last_error, nio_last_rx_len,
                 nio_last_expected_len);
        downshift = 1;
        break;
      }

      if (nio_debug_io && attempt < nio_io_retries)
        consolef("FN read retry u=%i lba=%li n=%i err=%i st=%i rx=%i exp=%i\n",
                 req->unit, sector, fetch, nio_last_error,
                 nio_last_status, nio_last_rx_len, nio_last_expected_len);
    }

    if (downshift)
      continue;

    if (!read_ok) {
      consolef("FN read fail u=%i lba=%li n=%i got=%i err=%i st=%i rx=%i exp=%i\n",
               req->unit, sector, fetch, bytes_read, nio_last_error,
               nio_last_status, nio_last_rx_len, nio_last_expected_len);
      break;
    }

    for (fill = 0; fill < fetch; fill++)
      cache_store(req->unit, sector + fill,
                  &buf[(done + fill) * SECTOR_SIZE]);

    done += fetch;
    sector += fetch;
  }
  if (!done)
    return ERROR_BIT | GENERAL_FAIL;

  req->io.count = done;
  return OP_COMPLETE;
}

uint16_t Input_no_wait_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Input_status_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Input_flush_cmd(SYSREQ far *req)
{
  consolef("FN INPUT FLUSH\n");
  return ERROR_BIT | GENERAL_FAIL;
}

uint16_t Output_cmd(SYSREQ far *req)
{
  uint16_t done = 0;
  uint32_t sector, sector_max;
  uint8_t far *buf = req->io.buffer_ptr;
  uint16_t bytes_written = 0;
  nio_disk_info_t info;


  if (req->unit >= FN_MAX_DEV) {
    consolef("Invalid Output unit: %i\n", req->unit);
    return ERROR_BIT | UNKNOWN_UNIT;
  }

  if (!disk_info_cached(req->unit, &info))
    return ERROR_BIT | NOT_READY;

  if (info.flags & NIO_DISK_INFO_READONLY)
    return ERROR_BIT | WRITE_PROTECT;

  if (req->length > 22)
    sector = req->io.start_sector_32;
  else
    sector = req->io.start_sector;

  if (fn_bpb_table[req->unit].num_sectors)
    sector_max = fn_bpb_table[req->unit].num_sectors;
  else
    sector_max = fn_bpb_table[req->unit].num_sectors_32;

#if 0
  consolef("WRITE SECTOR: %i 0x%08lx %i\n", req->length, sector, req->io.count);
#endif

  while (done < req->io.count) {
    uint16_t remaining = req->io.count - done;
    uint16_t batch = remaining > nio_batch_sectors ? nio_batch_sectors : remaining;
    bytes_written = 0;

    if (sector >= sector_max) {
      consolef("FN Invalid sector write %i on %i:\n", sector, req->unit);
      return ERROR_BIT | NOT_FOUND;
    }
    if ((uint32_t) batch > (sector_max - sector))
      batch = (uint16_t) (sector_max - sector);

    if (!nio_disk_write_sectors(unit_to_diskservice_slot(req->unit), sector, batch,
                                &buf[done * SECTOR_SIZE],
                                (uint16_t) (batch * SECTOR_SIZE),
                                &bytes_written) ||
        bytes_written < (uint16_t) (batch * SECTOR_SIZE)) {
      consolef("FN write fail u=%i lba=%li n=%i got=%i err=%i st=%i rx=%i exp=%i\n",
               req->unit, sector, batch, bytes_written, nio_last_error,
               nio_last_status, nio_last_rx_len, nio_last_expected_len);
      break;
    }

    {
      uint16_t fill;
      for (fill = 0; fill < batch; fill++)
        cache_update_or_invalidate(req->unit, sector + fill,
                                   &buf[(done + fill) * SECTOR_SIZE]);
    }

    done += batch;
    sector += batch;
  }
  if (!done)
    return ERROR_BIT | GENERAL_FAIL;

  req->io.count = done;
  return OP_COMPLETE;
}

uint16_t Output_verify_cmd(SYSREQ far *req)
{
  consolef("FN OUTPUT VERIFY\n");
  req->io.count = 0;
  return ERROR_BIT | GENERAL_FAIL;
}

uint16_t Output_status_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Output_flush_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Ioctl_output_cmd(SYSREQ far *req)
{
  return handle_ioctl_buffer(req);
}

uint16_t Dev_open_cmd(SYSREQ far *req)
{
  consolef("FN DEV OPEN\n");
  return ERROR_BIT | GENERAL_FAIL;
}

uint16_t Dev_close_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Remove_media_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Ioctl_cmd(SYSREQ far *req)
{
  consolef("IOCTL CALLED\n");
  return UNKNOWN_CMD;
}

uint16_t Get_l_d_map_cmd(SYSREQ far *req)
{
  consolef("FN GET LD MAP\n");
  req->ldmap.unit_code = 0;
  return ERROR_BIT | GENERAL_FAIL;
}

uint16_t Set_l_d_map_cmd(SYSREQ far *req)
{
  consolef("FN SET LD MAP\n");
  req->ldmap.unit_code = 0;
  return ERROR_BIT | GENERAL_FAIL;
}

uint16_t Unknown_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}
