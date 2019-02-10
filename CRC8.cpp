#include "CRC8.h"

byte CRC8::calculate(byte *data, byte len)
{
    byte crc = 0x00;
    while (len--) {
        byte extract = *data++;
        for (byte i = 8; i; i--) {
            byte sum = (crc ^ extract) & 0x01;
            crc >>= 1;
            if (sum) {
                crc ^= 0x8C;
            }
            extract >>= 1;
        }
    }
    return crc;
}
