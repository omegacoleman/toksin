#include <toksin.h>
#include <blocks.h>
#include <math.h>
#include <glib.h>

world c_world;

inline block gen_natural_block(uint32_t x, uint32_t y)
{
    if (y >= HORIZON + sin(x / 5.) * 5.)
    {
        return BLCK_DIRT;
    }
    else
    {
        return BLCK_AIR;
    }
}

void init_world()
{
    c_world.item_nr = 0;
    c_world.role_nr = 0;
    for(uint32_t i = 0; i < WORLD_HEIGHT; i++)
    {
        for(uint32_t j = 0; j < WORLD_WIDTH; j++)
        {
            c_world.solids[i * WORLD_WIDTH + j] = gen_natural_block(j, i);
        }
    }
}
