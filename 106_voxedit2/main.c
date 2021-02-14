#include "gfx.h"
#include <stdio.h>
#include <stdint.h>

/* create a 16x256x16 array of blocks */
uint8_t voxels[16 * 256 * 16];

int main() {
  gfx_start( "Voxedit2 by Anton Gerdelan\n", NULL, false );


  /* render a single cube for each, with instancing */

  /* oct-tree raycast function within chunk. ignore air tiles. */

  /* pop up tile chooser */

  /* button to save in either ply or a voxel format (just dims and tile types) */

  /* button to load from voxel format */

  gfx_stop();

  printf("normal exit\n");
  return 0;
}
