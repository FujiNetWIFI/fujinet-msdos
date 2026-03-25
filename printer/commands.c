#include "commands.h"
#include "fujinet.h"
#include "sys_hdr.h"
#include <fuji_f5.h>
#include <string.h>
#include <dos.h>

extern void End_code(void);

#define PRN_BUF_SIZE 255

static unsigned char prn_buf[PRN_BUF_SIZE];
static uint16_t prn_buf_len = 0;

int flush_prn_buf(void)
{
    int ok = 1;
    if (prn_buf_len > 0) {
        ok = fujiF5_write(FUJI_DEVICEID_PRINTER, FUJICMD_WRITE, FUJI_FIELD_NONE, 0, 0, prn_buf, prn_buf_len);
        prn_buf_len = 0;
    }
    return ok;
}

int prn_buf_add(unsigned char c)
{
    prn_buf[prn_buf_len++] = c;
    if (prn_buf_len >= PRN_BUF_SIZE)
        return flush_prn_buf();
    return 1;
}


uint16_t Media_check_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Build_bpb_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Ioctl_input_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Input_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
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
  return UNKNOWN_CMD;
}

uint16_t Output_cmd(SYSREQ far *req)
{
    if (!prn_buf_add(req->io.buffer_ptr[0]))
        return ERROR_BIT | NOT_READY;
    return OP_COMPLETE;
}

uint16_t Output_verify_cmd(SYSREQ far *req)
{
    return UNKNOWN_CMD;
}

uint16_t Output_status_cmd(SYSREQ far *req)
{
    return OP_COMPLETE;
}

uint16_t Output_flush_cmd(SYSREQ far *req)
{
    flush_prn_buf();
    return OP_COMPLETE;
}

uint16_t Ioctl_output_cmd(SYSREQ far *req)
{
    return UNKNOWN_CMD;
}

uint16_t Dev_open_cmd(SYSREQ far *req)
{
    return OP_COMPLETE;
}

uint16_t Dev_close_cmd(SYSREQ far *req)
{
    flush_prn_buf();
    return OP_COMPLETE;
}

uint16_t Remove_media_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Ioctl_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Get_l_d_map_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Set_l_d_map_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}

uint16_t Unknown_cmd(SYSREQ far *req)
{
  return UNKNOWN_CMD;
}
