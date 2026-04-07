#include "commands.h"
#include "fujinet.h"
#include "sys_hdr.h"
#include <fuji_f5.h>
#include <string.h>
#include <dos.h>

extern void End_code(void);

#define PRN_BUF_SIZE 255
#define AUTO_FLUSH_TICKS (18 * 5)  // 5 seconds at 18.2 Hz

static unsigned char prn_buf[PRN_BUF_SIZE];
static uint16_t prn_buf_len = 0;
static volatile uint16_t timer_counter = 0;
static volatile uint16_t last_activity = 0;
static volatile uint8_t flush_pending = 0;
static uint8_t far *indos_flag = NULL;
static uint8_t buffering_enabled = 0;

// Defined in iwrap.asm
extern uint16_t old_timer_off;
extern uint16_t old_timer_seg;
extern void timer_vect(void);
extern uint16_t old_idle_off;
extern uint16_t old_idle_seg;
extern void idle_vect(void);

int flush_prn_buf(void)
{
    int ok = 1;
    if (prn_buf_len > 0) {
        ok = fujiF5_write(FUJI_DEVICEID_PRINTER, FUJICMD_WRITE, FUJI_FIELD_NONE, 0, 0, prn_buf, prn_buf_len);
        prn_buf_len = 0;
        memset(prn_buf, 0, PRN_BUF_SIZE);
    }
    return ok;
}

int prn_buf_add(unsigned char c)
{
    prn_buf[prn_buf_len++] = c;
    if (!buffering_enabled)
        return flush_prn_buf();
    last_activity = timer_counter;
    flush_pending = 0;
    if (prn_buf_len >= PRN_BUF_SIZE)
        return flush_prn_buf();
    return 1;
}

// Called from INT 08h wrapper in iwrap.asm — DS=CS on entry
void timer_tick(void)
{
    timer_counter++;
    if (prn_buf_len > 0 && (timer_counter - last_activity) >= AUTO_FLUSH_TICKS) {
        if (indos_flag && *indos_flag == 0) {
            flush_prn_buf();
            last_activity = timer_counter;
        } else {
            flush_pending = 1;  // DOS busy — INT 28h will pick it up
        }
    }
}

// Called from INT 28h (DOS idle) — flush unconditionally
void idle_flush(void)
{
    flush_pending = 0;
    flush_prn_buf();
}

void install_timer_handler(void)
{
    void (__interrupt __far *old)(void);
    uint16_t indos_seg, indos_off;

    // Get InDOS flag address via INT 21h AH=34h
    _asm {
        mov  ah, 34h
        int  21h
        mov  indos_off, bx
        mov  indos_seg, es
    }
    indos_flag = (uint8_t far *)MK_FP(indos_seg, indos_off);

    old = _dos_getvect(0x08);
    old_timer_off = FP_OFF(old);
    old_timer_seg = FP_SEG(old);
    _dos_setvect(0x08, MK_FP(getCS(), (uint16_t)timer_vect));

    old = _dos_getvect(0x28);
    old_idle_off = FP_OFF(old);
    old_idle_seg = FP_SEG(old);
    _dos_setvect(0x28, MK_FP(getCS(), (uint16_t)idle_vect));

    // Buffer + timer + INT 28h require DOS 3.0+
    _asm {
        mov  ah, 30h
        int  21h
        mov  buffering_enabled, al
    }
    buffering_enabled = (buffering_enabled >= 3) ? 1 : 0;

    timer_counter = 0;
    last_activity = 0;
    flush_pending = 0;
}

void uninstall_timer_handler(void)
{
    if (old_timer_seg) {
        _dos_setvect(0x08, (void (__interrupt __far *)(void))MK_FP(old_timer_seg, old_timer_off));
        old_timer_seg = 0;
        old_timer_off = 0;
    }
    if (old_idle_seg) {
        _dos_setvect(0x28, (void (__interrupt __far *)(void))MK_FP(old_idle_seg, old_idle_off));
        old_idle_seg = 0;
        old_idle_off = 0;
    }
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
