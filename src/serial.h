#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "types.h"

typedef enum
{
    BAUDRATE_300  = 0xC0,
    BAUDRATE_1200 = 0x80,
    BAUDRATE_2400 = 0x40,
    BAUDRATE_4800 = 0
} Baudrate;

#define MSG_DONTWAIT 0x1  // Nonblocking IO
#define MSG_WAITALL  0x2  // Wait for a full request

#define PORT_3 0x0002

void COM_enable(u16 port, Baudrate baudrate);
void COM_disable();
u16 COM_send(void *buf, u16 len, u8 flags);
u16 COM_recv(void *buf, u16 len, u8 flags);

#endif // _SERIAL_H_