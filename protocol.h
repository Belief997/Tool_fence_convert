#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "types.h"
#pragma pack(push, 1)

typedef enum
{
    ENDIAN_BIG,
    ENDIAN_SMALL,
}ENUM_ENDIAN;

#define nettohs(n) ((u16)((((n) << 8) & 0xff00) | (((n) >> 8) & 0x00ff)))
#define nettohl(n) ((((n) & 0x000000ff) << 24) | (((n) << 8) & 0x00ff0000) | \
                   (((n) >> 8) & 0x0000ff00) | (((n) >> 24) & 0x000000ff))

#define ed_tlr_32(x, e) ( (e == ENDIAN_SMALL) ? (x):(nettohl(x)) )
#define ed_tlr_16(x, e) ( (e == ENDIAN_SMALL) ? (x):(nettohs(x)) )

typedef struct
{
    float longitude;
    float latitude;
}__attribute__((__packed__)) FENCE_POINT;

typedef struct
{
    u8 mode;
    u16 nodeNum;
    FENCE_POINT *fencePoint;
}__attribute__((__packed__)) FENCE_POLY;

typedef struct
{
    u8 signature[10];
    u32 version;
    u8 fenceNum;
    FENCE_POLY fencePoly[16];
    FENCE_POINT points[0];
}__attribute__((__packed__)) FENCE;

#pragma pack(pop)
#endif
