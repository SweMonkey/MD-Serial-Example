#include <genesis.h>
#include "serial.h"


int main()
{
    u16 num;
    s16 y = 0;
    char echoback[64];
    char printmsg[256];

    memset(echoback, 0, 64);
    memset(printmsg, 0, 256);

    COM_enable(PORT_3, BAUDRATE_4800);   // Enable serial mode on port 3

    while (1)
    {        
        num = COM_recv(echoback, 64, MSG_DONTWAIT); // Was anything was sent to us? If so, try to read it into back into the buffer "echoback" with a limit of max 64 characters

        COM_send(echoback, num, MSG_WAITALL);       // If anything was sent to us, then send it straight back

        // Wait until we receive a newline from the remote connection
        if (num && echoback[num-1] == '\n')
        {
            strcat(printmsg, echoback); // Concatenate the final part of the message to our print buffer

            // Print our full message that we received to screen
            VDP_drawText(" Received message:                     ", 0, y);
            VDP_drawText(printmsg, 19, y++);

            VDP_setVerticalScroll(BG_A, -224 + (y<<3)); // Scroll the screen upwards 1 line

            y %= 32;

            strclr(printmsg);
        }
        else if (num)
        {
            strcat(printmsg, echoback); // Partial message was sent to us, concatenate the string to our temporary print buffer
        }

        memset(echoback, 0, 64);
        SYS_doVBlankProcess();
    }

    return 0;
}
