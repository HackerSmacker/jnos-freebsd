#include "global.h"
#ifdef APRSD
#include <ctype.h>

#define APRSDATDIR "/jnos/aprs/users"

int aprs_setattribute( char *callsign, char *attribute, char *newval )
{
  char filename[1024], tmpfilename[1024];
  char buf[1024], *oldval;
  FILE *fp, *tmpfp;
  int found=0;
  
  sprintf( filename, "%s/%s.dat", APRSDATDIR, callsign );
  sprintf( tmpfilename, "%s/%s.dat.tmp", APRSDATDIR, callsign );
  
  fp = fopen( filename, "r" );
  /* Okay not to have an existing file.  We'll create a new one. */
  
  tmpfp = fopen( tmpfilename, "w" );
  if( tmpfp == NULL ){
    fclose( fp );
    return -1;
  }
  
  /* copy each line, but if we find the attribute, update it */
  while( fp && fgets( buf, 1023, fp ) != NULL ){
    
    /* copy comments */
    if( buf[0] == '#' ){
      fprintf( tmpfp, "%s", buf );
      continue;
    }
    
    /* look for attribute */
    oldval = strchr( buf, ':' );
    if( oldval ){
      *oldval = '\0';
      oldval++;
      
      if( !strcmp( buf, attribute )){
	found = 1;
	fprintf( tmpfp, "%s:%s\n", buf, newval );
      } else {
	fprintf( tmpfp, "%s:%s\n", buf, oldval );
      }
    } else
      fprintf( tmpfp, "%s", buf );
  }
  
  if( !found ){
    fprintf( tmpfp, "%s:%s\n", attribute, newval );
  }

  if( fp )
    fclose( fp );
  
  fclose( tmpfp );
  
  /* replace the file with the new one */
  rename( tmpfilename, filename );
  
  return 0;
}

char *aprs_getattribute( char *callsign, char *attribute )
{
  char filename[1024];
  char buf[1024], *oldval;
  FILE *fp;
  char *value=NULL;
  
  sprintf( filename, "%s/%s.dat", APRSDATDIR, callsign );
  
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return NULL;
  
  while( fgets( buf, 1023, fp ) != NULL ){
    
    /* ignore comments */
    if( buf[0] == '#' ){
      continue;
    }
    
    /* look for attribute */
    oldval = strchr( buf, ':' );
    if( oldval ){
      *oldval = '\0';
      oldval++;
      
      if( !strcmp( buf, attribute )){
	value = j2strdup( oldval );
	break;
      }
    }
  }
  
  fclose( fp );
  
  return value;
}

int aprs_clearattribute( char *callsign, char *attribute )
{
  char filename[1024], tmpfilename[1024];
  char buf[1024], *oldval;
  FILE *fp, *tmpfp;
  
  sprintf( filename, "%s/%s.dat", APRSDATDIR, callsign );
  sprintf( tmpfilename, "%s/%s.dat.tmp", APRSDATDIR, callsign );
  
  fp = fopen( filename, "r" );
  if( fp == NULL )
    /* Nothing to clear. */
    return 0;

  tmpfp = fopen( tmpfilename, "w" );
  if( tmpfp == NULL ){
    fclose( fp );
    return -1;
  }

  /* copy each line, but if we find the attribute, update it */
  while( fgets( buf, 1023, fp ) != NULL ){

    /* copy comments */
    if( buf[0] == '#' ){
      fprintf( tmpfp, "%s", buf );
      continue;
    }

    /* look for attribute */
    oldval = strchr( buf, ':' );
    if( oldval ){
      *oldval = '\0';
      oldval++;

      /* omit it if we find it */
      if( strcmp( buf, attribute )){
	fprintf( tmpfp, "%s:%s\n", buf, oldval );
      }
    } else
      fprintf( tmpfp, "%s", buf );
  }

  fclose( fp );
  fclose( tmpfp );

  /* replace the file with the new one */
  rename( tmpfilename, filename );
  
  return 0;
}

int aprs_setuserssid( char *callsign, char *ssidstr )
{
  int ssid, len, ret;

  len = strlen( ssidstr );
  //  tprintf( "Got ssidstr '%s'\n", ssidstr );
  
  if( len < 1 || len > 2 )
    return -1;

  if( !isdigit( ssidstr[0] ) || (ssidstr[1] && !isdigit( ssidstr[1] )))
    return -1;

  ssid = atoi( ssidstr );
  if( ssid < 1 || ssid > 15 )
    return -1;

  ret = aprs_setattribute( callsign, "ssid", ssidstr );
  if( ret == -1 )
    return -1;
  
  return ssid;
}

int aprs_getuserssid( char *callsign )
{
  int ssid;
  char *p;

  p = aprs_getattribute( callsign, "ssid" );
  if( p == NULL )
    return -1;

  ssid = atoi( p );
  free( p );

  return ssid;
}

/* Receive "tomatch", a callsign possibly with an SSID on it (e.g. KZ7ZZZ-12),
 * look up the SSID set for "callsign", and see if there's a match. */

int aprs_checkuserssid( char *callsign, char *tomatch )
{
  char buf[10];
  int ssid;

  ssid = aprs_getuserssid( callsign );
  if( ssid > 0 )
    sprintf( buf, "%s-%d", callsign, ssid );
  else
    sprintf( buf, "%s", callsign );

  if( !strcasecmp( buf, tomatch ))
    return 1;

  return 0;
}

 
#endif
