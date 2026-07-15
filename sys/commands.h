#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "sys_hdr.h"
#include <stdint.h>

#ifdef FUJINET_TRANSPORT_NIO
#include "ioctl.h"
#endif

extern __segment getCS(void);
#pragma aux getCS = \
    "mov ax, cs";

extern DOS_BPB fn_bpb_table[];
extern DOS_BPB *fn_bpb_pointers[];

extern uint16_t Init_cmd(SYSREQ far *req);
extern uint16_t Media_check_cmd(SYSREQ far *req);
extern uint16_t Build_bpb_cmd(SYSREQ far *req);
extern uint16_t Ioctl_input_cmd(SYSREQ far *req);
extern uint16_t Input_cmd(SYSREQ far *req);
extern uint16_t Input_no_wait_cmd(SYSREQ far *req);
extern uint16_t Input_status_cmd(SYSREQ far *req);
extern uint16_t Input_flush_cmd(SYSREQ far *req);
extern uint16_t Output_cmd(SYSREQ far *req);
extern uint16_t Output_verify_cmd(SYSREQ far *req);
extern uint16_t Output_status_cmd(SYSREQ far *req);
extern uint16_t Output_flush_cmd(SYSREQ far *req);
extern uint16_t Ioctl_output_cmd(SYSREQ far *req);
extern uint16_t Dev_open_cmd(SYSREQ far *req);
extern uint16_t Dev_close_cmd(SYSREQ far *req);
extern uint16_t Remove_media_cmd(SYSREQ far *req);
extern uint16_t Ioctl_cmd(SYSREQ far *req);
extern uint16_t Get_l_d_map_cmd(SYSREQ far *req);
extern uint16_t Set_l_d_map_cmd(SYSREQ far *req);
extern uint16_t Unknown_cmd(SYSREQ far *req);

#ifdef FUJINET_TRANSPORT_NIO
extern uint16_t nio_handle_control_call(fuji_ioctl_nio_call far *call,
                                        uint8_t unit);
#endif

#endif /* _COMMANDS_H */
