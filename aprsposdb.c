
#include "global.h"

#ifdef	APRSD

#include <stdio.h>
#include "aprs.h"

#include "files.h"

void aprs_db_add_position (char *srccall, char *data)
{
	char filename[100];

	FILE *fp;

	sprintf (filename, "%s/db/positions/%s", APRSdir, srccall);

	if ((fp = fopen (filename, "w")) != NULLFILE)
	{
		fprintf (fp, "%s %.*s", srccall, 18, data);

		fclose (fp);
	}

	return;
}

int aprs_getlocation( char *srccall, double *latitude, double *longitude )
{
  FILE *fp;
  char filename[100];
  char buf[100], *cp;

  if( latitude == NULL || longitude == NULL )
    return 0;

  sprintf (filename, "%s/db/positions/%s", APRSdir, srccall);
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return 0;

  if( fgets( buf, 99, fp ) == NULL ){
    fclose( fp );
    return 0;
  }

  fclose( fp );
    
  cp = strchr( buf, ' ' );
  if( cp == NULL )
    return 0;
  
  cp++;
  
  *latitude = (((double)((cp[5]-'0') * 10 + (cp[6]-'0')) / 100.0) +
	       (double)((cp[2]-'0') * 10 + (cp[3]-'0'))) / 60.0 +
    (double)((cp[0]-'0') * 10 + (cp[1]-'0'));
  if( cp[7] == 'S' )
    *latitude = -*latitude;
  
  cp += 9;
  
  *longitude = (((double)((cp[6]-'0') * 10 + (cp[7]-'0')) / 100.0) +
		(double)((cp[3]-'0') * 10 + (cp[4]-'0'))) / 60.0 +
    (double)((cp[0]-'0') * 100 + (cp[1]-'0') * 10 + (cp[2]-'0'));
  if( cp[8] == 'W' )
    *longitude = -*longitude;

  //  aprslog( -1, "geometry: converted %s to %f x %f", buf, *latitude, *longitude );
  
  return 1;
}

int aprs_withindistance( char *callsign, double miles )
{
  double latitude, longitude, distance;

  if( aprs_getlocation( callsign, &latitude, &longitude )){
    distance = earth_distance( latitude, longitude, 42.4335, -71.4495 );

    //    aprslog( -1, "distance to %s = %f miles", callsign, miles );
    
    if( distance < miles )
      return 1;
  }

  return 0;
}

#endif


