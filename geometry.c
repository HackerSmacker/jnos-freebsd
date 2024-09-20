#include <stdio.h>
#include <math.h>

double sphere_distance( double lat1, double long1, double lat2, double long2 );

#if 0
int main( int argc, char **argv )
{
  double d;
  
  d = sphere_distance( 42.36, -71.06, 42.05, -92.90 );

  printf( "d: %f\n", d );

  return 0;
}
#endif

double earth_distance( double lat1, double long1, double lat2, double long2 )
{
  double R, phi1, phi2, deltaphi, deltalambda, a, c, d;

  /* Haversine formula */
  R = 6371000.0 / 1609.0;
  phi1 = lat1 * 3.14159 / 180.0;
  phi2 = lat2 * 3.14159 / 180.0;
  deltaphi = (lat2 - lat1) * 3.14159 / 180.0;
  deltalambda = (long2 - long1) * 3.14159 / 180.0;
  a = sin( deltaphi / 2.0 ) * sin( deltaphi / 2.0 ) +
    cos( phi1 ) * cos( phi2 ) *
    sin( deltalambda / 2.0 ) * sin( deltalambda / 2.0 );
  c = 2.0 * atan( sqrt( a ) / sqrt( 1.0 - a));
  d = R * c;

  return d;
}
