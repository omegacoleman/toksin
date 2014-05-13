#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <inttypes.h>
typedef uint32_t magic_code;
typedef uint32_t operation_code;

static const magic_code ___mc = 0x544B534E;

#define MAGIC ((___mc))

#define OPC_CLOSE				0x00000000
#define OPC_PING				0x00000001
#define RSP_PONG				0x00000002
#define OPC_GET_RANGE			0x00000003
#define RSP_SET_BLOCK			0x00000004
#define OPC_DIG					0x00000005
#define RSP_FLUSH_REQUEST		0x00000006
#define OPC_PLACE				0x00000007
#define RSP_FLUSH_DONE			0x00000008
#define RSP_SHOW_SPIRIT			0x00000009
#define RSP_SPIRIT_CHANGE_ATTR	0x00000010
#define RSP_ATOMIC_UPDATE		0x00000011

typedef struct _op_get_range
{
    uint32_t xa, ya, xb, yb;
} op_get_range;

typedef struct _op_dig
{
    uint32_t xa, ya;
    // uint32_t tool;
} op_dig;

typedef struct _op_place
{
    uint32_t xa, ya;
    // uint32_t what;
} op_place;

typedef struct _rsp_set_block
{
    uint32_t startx, starty, amount;
} rsp_set_block;
// Send blocks with it. Send a line at one time.

typedef struct _rsp_atomic_update
{
    uint32_t startx, starty;
	uint64_t atom;
} rsp_atomic_update;

typedef struct _rsp_show_spirit
{
	uint32_t pos_x, pos_y;
	uint16_t pos_cent_x, pos_cent_y;
	uint64_t res_musk;
} rsp_show_spirit;


#endif // PROTOCOL_H
