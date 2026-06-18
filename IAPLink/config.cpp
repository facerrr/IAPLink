#include "config.h"

config::config() {}


quint16 Cal_CRC16(quint8* data, int offset, quint32 size){
    quint8 u8Cnt;
    quint16 u16CrcResult = 0xA28C;
    quint32 u32Offset = (quint32)offset;
    while(size != 0){
        u16CrcResult ^= data[u32Offset++];
        for(u8Cnt = 0; u8Cnt < 8; u8Cnt++){
            if((u16CrcResult & 0x1) == 0x1){
                u16CrcResult >>= 1;
                u16CrcResult ^= 0x8408;
            }else{
                u16CrcResult >>= 1;
            }
        }
        size--;
    }
    u16CrcResult = (quint16)(~u16CrcResult);
    return u16CrcResult;
}


quint8 Cal_ADD8(quint8* data, int offset, quint32 size){
    quint8 checksum = 0;
    for (quint32 i = 0; i < size; ++i) {
        checksum += data[offset + i];
    }
    return checksum;
}
