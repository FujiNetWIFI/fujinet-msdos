#include "commands.h"
#include "fujinet.h"
#include "sys_hdr.h"
#include <fuji_f5.h>
#include <string.h>
#include <dos.h>

extern void End_code(void);


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
    char c = req->io.buffer_ptr[0];
    if (!fujiF5_write(FUJI_DEVICEID_PRINTER, FUJICMD_WRITE, FUJI_FIELD_NONE, 0, 0, &c, 1))
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
    return UNKNOWN_CMD;
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
