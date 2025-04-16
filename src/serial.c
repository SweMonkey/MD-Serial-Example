#include "serial.h"
#include "sys.h"
#include "vdp.h"
#include "joy.h"
#include "z80_ctrl.h"
#include "config.h"

#define PORT1_SCTRL 0xA10013
#define PORT1_SRx   0xA10011
#define PORT1_STx   0xA1000F

#define PORT2_SCTRL 0xA10019
#define PORT2_SRx   0xA10017
#define PORT2_STx   0xA10015

#define PORT3_SCTRL 0xA1001F
#define PORT3_SRx   0xA1001D
#define PORT3_STx   0xA1001B

#define COM_BUFFER_LEN 64

#if (HALT_Z80_ON_IO != 0)
#define Z80_STATE_DECL      u16 z80state;
#define Z80_STATE_SAVE      z80state = Z80_getAndRequestBus(TRUE);
#define Z80_STATE_RESTORE   if (!z80state) Z80_releaseBus();
#else
#define Z80_STATE_DECL
#define Z80_STATE_SAVE
#define Z80_STATE_RESTORE
#endif

// Serial Comm.
static vu32 com_ctrl = 0;
static vu32 com_rx = 0;
static vu32 com_tx = 0;
void COM_RxINT();

// Rx Buffer
static u8 rxbuf[COM_BUFFER_LEN];
static u16 rxbuf_head;
static u16 rxbuf_tail;
void COM_push(u8 byte);
bool COM_pop(u8 *byte);
void COM_flush();


void COM_enable(u16 port, Baudrate baudrate)
{
    if (com_ctrl != 0)
    {
        COM_disable();
    }
    
    switch (port)
    {
        case PORT_1:
            com_ctrl = PORT1_SCTRL;
            com_rx   = PORT1_SRx;
            com_tx   = PORT1_STx;
        break;
        
        case PORT_2:
            com_ctrl = PORT2_SCTRL;
            com_rx   = PORT2_SRx;
            com_tx   = PORT2_STx;
        break;
        
        case PORT_3:
            com_ctrl = PORT3_SCTRL;
            com_rx   = PORT3_SRx;
            com_tx   = PORT3_STx;
        break;
        
        default: return;
    }

    JOY_setSupport(port, JOY_SUPPORT_OFF);
    
    *((vu8*)com_ctrl) = baudrate | 0x38;    // Set baudrate, TR and TL to serial mode and enable triggering EXT interrupt
    
    SYS_setExtIntCallback(COM_RxINT);       // Set external interrupt callback
    VDP_setReg(0xB, VDP_getReg(0xB) | 0x8); // Enable VDP EXT interrupt
    SYS_setInterruptMaskLevel(1);           // Enable all interrupts on the 68k                  - hmm ?
}

void COM_disable()
{
    VDP_setReg(11, VDP_getReg(11) & ~0x08); // Disable VDP EXT interrupt
    SYS_setInterruptMaskLevel(3);           // Disable EXT interrupt on the 68k
    SYS_setExtIntCallback(NULL);
    
    *((vu8*)com_ctrl) = 0;    // Reset serial control register
    
    com_ctrl = 0;
    com_rx   = 0;
    com_tx   = 0;
    
    COM_flush();
}

void COM_RxINT()
{
    Z80_STATE_DECL
    Z80_STATE_SAVE

    if ((*(vu8*)com_ctrl & 6) != 2) return;   // Check Ready/RxError flag in serial control register, Bail if no byte is ready or if there was an Rx error
    
    COM_push(*(vu8*)com_rx);
    
    Z80_STATE_RESTORE
}

u16 COM_send(void *buf, u16 len, u8 flags)
{
    if (!com_ctrl) return 0;  // ...

    u8 *data = (u8*)buf;
    u16 rb = 0;
    
    Z80_STATE_DECL
    Z80_STATE_SAVE

    while (rb < len)
    {
        while (*(vu8*)com_ctrl & 1) // while Txd full = 1
        {
            if ((flags & MSG_DONTWAIT) && ((flags & MSG_WAITALL) == 0)) goto QuitEarly;  // Return early if tx is not ready unless told to wait until the full amount is sent
        }
    
        *(vu8*)com_tx = data[rb];

        rb++;
    }

    QuitEarly:
    
    Z80_STATE_RESTORE

    return rb;
}

u16 COM_recv(void *buf, u16 len, u8 flags)
{
    u8 *data = (u8*)buf;
    u16 rb = 0;
    bool result = FALSE;

    while (rb < len)
    {
        result = COM_pop(&data[rb]);

        if (result) rb++;

        if (!result && (flags & MSG_DONTWAIT) && ((flags & MSG_WAITALL) == 0)) break;
    }
    
    return rb;
}

// -- Buffer --
void COM_push(u8 byte)
{
    u16 next;

    next = rxbuf_head + 1;     // Next is where head will point to after this write.
    if (next == COM_BUFFER_LEN)
        next = 0;

    if (next == rxbuf_tail)    // If the head + 1 == tail then the buffer is full
        return;

    rxbuf[rxbuf_head] = byte;  // Store byte and then move the
    rxbuf_head = next;         // head to next data offset.

    return;
}

bool COM_pop(u8 *byte)
{
    u16 next;

    if (rxbuf_head == rxbuf_tail)  // If the head == tail, we don't have any data
        return FALSE;

    next = rxbuf_tail + 1;         // Next is where tail will point to after this read.
    if (next == COM_BUFFER_LEN) next = 0;

    *byte = rxbuf[rxbuf_tail];     // Read data and then move the
    rxbuf_tail = next;             // tail to next offset.

    return TRUE;  // Indicate successful pop.
}

void COM_flush()
{
    rxbuf_head = 0;
    rxbuf_tail = 0;
}
