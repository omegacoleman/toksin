#include "toksin.hpp"
#include "blocks.hpp"
#include <math.h>
#include <glib.h>
#include <gio/gio.h>
#include "main.hpp"

#define NEWEST_BACKUP_FN "C_CWORLD.bin"

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

void dump_world_to_file()
{
	char fn[255] = "";
	g_snprintf(fn, 255, "CWORLD_%lld.bin", g_get_real_time());
	dump_world_to(fn);
	dump_world_to(NEWEST_BACKUP_FN);
}

void dump_world_to(char *fn)
{
    GFile *fd;
	GOutputStream *ostream;
	GError *error = NULL;
	g_message("Dumping world to backup file %s..", fn);
	fd = g_file_new_for_path(fn);
	ostream = G_OUTPUT_STREAM(g_file_replace(fd, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error));
	if(error != NULL)
	{
		g_warning("Failed to dump world to backup file(1) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	gsize bytes_written;
	g_output_stream_write_all(ostream, &(c_world.solids), WORLD_HEIGHT * WORLD_WIDTH * sizeof(block), &bytes_written, NULL, &error);
	if(error != NULL)
	{
		g_warning("Failed to dump world to backup file(2) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	g_output_stream_flush(ostream, NULL, &error);
	if(error != NULL)
	{
		g_warning("Failed to dump world to backup file(3) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	g_output_stream_close(ostream, NULL, &error);
	if(error != NULL)
	{
		g_warning("Failed to dump world to backup file(4) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	g_object_unref(fd);
}

void try_restore_world()
{
	if (g_file_test(NEWEST_BACKUP_FN, G_FILE_TEST_EXISTS))
	{
		restore_world_from(NEWEST_BACKUP_FN);
	} else {
		g_message("%s not exist. Using default init map.", NEWEST_BACKUP_FN);
		init_world();
	}
}

void restore_world_from(char *fn)
{
    GFile *fd;
	GInputStream *istream;
	GError *error = NULL;
	g_message("Restoring world from backup file %s..", fn);
	fd = g_file_new_for_path(fn);
	istream = G_INPUT_STREAM(g_file_read(fd, NULL, &error));
	if(error != NULL)
	{
		g_warning("Failed to restore world from backup file(1) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	gsize bytes_written;
	g_input_stream_read_all(istream, &(c_world.solids), WORLD_HEIGHT * WORLD_WIDTH * sizeof(block), &bytes_written, NULL, &error);
	if(error != NULL)
	{
		g_warning("Failed to restore world from backup file(2) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	g_input_stream_close(istream, NULL, &error);
	if(error != NULL)
	{
		g_warning("Failed to restore world from backup file(3) -- better stop. Error: %s", error->message);
		g_clear_error(&error);
		return;
	}
	g_object_unref(fd);
}

void anim_world()
{
	for(int i = 0; i < WORLD_HEIGHT; i++)
	{
		for(int j = 0; j < WORLD_WIDTH; j++)
		{
			if (i >= 1)
			{
				if ((c_world.solids[i * WORLD_WIDTH + j] == BLCK_DIRT) && (c_world.solids[(i - 1) * WORLD_WIDTH + j] == BLCK_AIR))
				{
					do_queued_atomic_update(j, i, BLCK_GRASS);
				}else if ((c_world.solids[i * WORLD_WIDTH + j] == BLCK_GRASS) && (c_world.solids[(i - 1) * WORLD_WIDTH + j] != BLCK_AIR))
				{
					do_queued_atomic_update(j, i, BLCK_DIRT);
				}
			}
		}
	}
}
