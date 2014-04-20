#ifndef TOKSIN_H
#define TOKSIN_H

typedef unsigned long block; // Shall be long enough to contain min_block_type in blocks.h

#define BLOCK_MASK 0xf0000000
#define ITEM_MASK 0x50000000
#define ROLE_MASK 0xe0000000

#define _GENERATE_BLOCK(b) (((block)(b | BLOCK_MASK)))
#define _GENERATE_ITEM(b) (((block)(b | ITEM_MASK)))
#define _GENERATE_ROLE(b) (((block)(b | ROLE_MASK)))

#define WORLD_HEIGHT 0xff
#define WORLD_WIDTH 0xffff
#define WORLD_ITEM_POOL_WIDTH 0xfff
#define WORLD_ROLE_POOL_WIDTH 0xff

#define HORIZON 0x8f

typedef struct _role
{
	char name[64];
	unsigned int pos_x;
	unsigned int pos_y;
} role;

typedef struct _placed_item
{
	block item;
	unsigned int pos_x;
	unsigned int pos_y;
} placed_item;

typedef struct _world
{
	block solids[WORLD_HEIGHT * WORLD_WIDTH];
	placed_item items[WORLD_ITEM_POOL_WIDTH];
	role roles[WORLD_ROLE_POOL_WIDTH];
	unsigned int item_nr;
	unsigned int role_nr;
} world;

extern world c_world;

void init_world();

#endif // TOKSIN_H
