#include <toksin.h>
#include <blocks.h>

world c_world;

inline block gen_natural_block(unsigned int x, unsigned int y)
{
	if (y >= HORIZON)
	{
		return BLCK_DIRT;
	} else {
		return BLCK_AIR;
	}
}

void init_world()
{
	c_world.item_nr = 0;
	c_world.role_nr = 0;
	for(unsigned int i = 0; i < WORLD_HEIGHT; i++)
	{
		for(unsigned int j = 0; j < WORLD_WIDTH; j++)
		{
			c_world.solids[i * WORLD_WIDTH + j] = gen_natural_block(j, i);
		}
	}
}
