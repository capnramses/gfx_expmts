
#include <stdio.h>

/* find maximum of a and b */
#define MAX( a, b ) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

/* absolute value of a */
#define ABS( a ) ( ( ( a ) < 0 ) ? -( a ) : ( a ) )

/* take sign of a, either -1, 0, or 1 */
#define ZSGN( a ) ( ( ( a ) < 0 ) ? -1 : ( a ) > 0 ? 1 : 0 )

void point3d( x, y, z ) int x, y, z;
{ /* output the point as you see fit */
  printf( "x,y,z: {%i,%i,%i}\n", x, y, z );
}

void line3d( x1, y1, x2, y2, z1, z2 ) int x1, y1, x2, y2, z1, z2;
{
  int xd, yd, zd;
  int x, y, z;
  int ax, ay, az;
  int sx, sy, sz;
  int dx, dy, dz;

  dx = x2 - x1;
  dy = y2 - y1;
  dz = z2 - z1;

  ax = ABS( dx ) << 1;
  ay = ABS( dy ) << 1;
  az = ABS( dz ) << 1;

  sx = ZSGN( dx );
  sy = ZSGN( dy );
  sz = ZSGN( dz );

  x = x1;
  y = y1;
  z = z1;

  /* x dominant */
  if ( ax >= MAX( ay, az ) ) {
    yd = ay - ( ax >> 1 );
    zd = az - ( ax >> 1 );
    for ( ;; ) {
      point3d( x, y, z );
      if ( x == x2 ) { return; }

      if ( yd >= 0 ) {
        y += sy;
        yd -= ax;
      }

      if ( zd >= 0 ) {
        z += sz;
        zd -= ax;
      }

      x += sx;
      yd += ay;
      zd += az;
    }
    /* y dominant */
  } else if ( ay >= MAX( ax, az ) ) {
    xd = ax - ( ay >> 1 );
    zd = az - ( ay >> 1 );
    for ( ;; ) {
      point3d( x, y, z );
      if ( y == y2 ) { return; }

      if ( xd >= 0 ) {
        x += sx;
        xd -= ay;
      }

      if ( zd >= 0 ) {
        z += sz;
        zd -= ay;
      }

      y += sy;
      xd += ax;
      zd += az;
    }
  } else if ( az >= MAX( ax, ay ) ) /* z dominant */
  {
    xd = ax - ( az >> 1 );
    yd = ay - ( az >> 1 );
    for ( ;; ) {
      point3d( x, y, z );
      if ( z == z2 ) { return; }

      if ( xd >= 0 ) {
        x += sx;
        xd -= az;
      }

      if ( yd >= 0 ) {
        y += sy;
        yd -= az;
      }

      z += sz;
      xd += ax;
      yd += ay;
    }
  }
}

int main() {
  // NOTE unusual param oder: x1,y1,x2,y2,z1,z2
  line3d( 0, 0, 4, 0, 0, 0 );
  return 0;
}
