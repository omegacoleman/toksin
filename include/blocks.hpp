#ifndef BLOCKS_H
#define BLOCKS_H

#include <inttypes.h>
#include <glib.h>

typedef uint64_t min_block_type;

#define WORLD_HEIGHT 0xff
#define WORLD_WIDTH 0x6ff

#define BLCK_AIR 0x0000
#define BLCK_DIRT 0x0001
#define BLCK_BRICK 0x0002
#define BLCK_GRASS 0x0003

#endif // BLOCKS_H
