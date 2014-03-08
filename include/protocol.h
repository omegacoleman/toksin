#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef unsigned int magic_code;
typedef unsigned int operation_code;

static const magic_code ___mc = 0x544B534E;

#define MAGIC ((___mc))

#define OPC_CLOSE 0x00000000
#define OPC_PING 0x00000007
#define RSP_PONG 0x00000008
#define OPC_GET_RANGE 0x00000003

typedef struct _op_get_range
{
    long long xa;
    long long ya;
    long long xb;
    long long yb;
} op_get_range;


#endif // PROTOCOL_H
