#ifndef FUSE_LIBRETRO_TAPEDECK_H
#define FUSE_LIBRETRO_TAPEDECK_H

#include <stdint.h>
#include <libretro.h>

void tapedeck_reset(void);
/* Match a cassette photo to the loaded tape's filename. */
void tapedeck_load_artwork(const char *tape_path, const char *system_dir);
/* Returns true while the deck owns the controller/keyboard navigation. */
int tapedeck_poll(retro_input_state_t input);
int tapedeck_visible(void);
int tapedeck_owns_input(void);
int tapedeck_poke_visible(void);
void tapedeck_draw(uint16_t *pixels, unsigned stride,
                   unsigned width, unsigned height);

#endif
