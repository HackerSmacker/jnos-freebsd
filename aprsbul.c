/* APRS
 * 
 * Handle bulletins
 */

#include "global.h"
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "aprs.h"
#include "files.h"
#ifdef  APRSD

static int bulletin_timeout = 3600 * 24;
static int announcement_timeout = 3600 * 168;
static int nwsbulletin_timeout = 60 * 15;


struct aprs_bulletin {
  struct aprs_bulletin *next;

  int type;
  char callsign[10];
  char bulid; 
  char group[6]; /* len 5 chars */
  char msg[300];  /* max len 67 chars */
  char msgid[6]; /* max len 5 chars */
  
  int expiration;
  char expiredate[13];
  char category[64];
  char *nwsmsg;
};

struct aprs_bulletin *Aprs_bulletins=NULL;
struct aprs_bulletin *Aprs_groupbulletins=NULL;
struct aprs_bulletin *Aprs_announcements=NULL;


struct county {
  char *county;
  char statecode[2];
  char *state;
};


struct county **SAMEcodes = NULL;
char **States=NULL;

int aprs_bul_register( char *callsign, char *recipient, char *msg );
struct aprs_bulletin *aprs_bul_find( char *callsign, char bulid, char *group );
struct aprs_bulletin *aprs_bul_findnws( char *callsign, char *group, char *msg );
void aprs_showbuls( void );
void aprs_bul_showlist( int type );
int aprs_getbulletintimeoutbytype( int type );
int aprs_bul_iscurrent( struct aprs_bulletin *bul );
int aprs_getbulletinnwstimeout( char *msg, char *expiredate );
int nws_addtext( char **p, int *size, int linelen, int indent, int *pos, char *text, char delim );
int aprs_processnwslocations( char *msg, char *buf, int len, int linelen, int indent );
struct county *same_lookupcode( char *code );
void aprs_bulletinadd( struct aprs_bulletin *bul );
int aprs_bulletincompare( struct aprs_bulletin *a, struct aprs_bulletin *b );

void aprs_bulletincheck( char *srccall, char *callsign, char *data )
{
  int bultype;

  /* Look for an APRS bulletin */
  bultype = aprs_isbulletin( callsign );
  if( bultype == APRS_BULLETIN ){
    aprs_bul_register( srccall, callsign, data );
  } else if( bultype == APRS_NWSBULLETIN ){
    //		  if( callsign_can_nws( srccall ))
    aprs_bul_register( srccall, callsign, data );
  }
  
  return;
}


int aprs_isbulletin( char *callsign )
{
  if( !strncasecmp( callsign, "BLN", 3 ))
    return APRS_BULLETIN;

  if( !strncasecmp( callsign, "NWS-", 4 ))
    return APRS_NWSBULLETIN;

  return 0;
}

int aprs_bul_register( char *callsign, char *recipient, char *msg )
{
  struct aprs_bulletin *p=NULL;
  char bulid, *c, group[6];
  int newbul = 0;
  int len, type;

  aprslog( -1, "aprs_bul_register: callsign (%s) recipient (%s) msg (%s)", callsign, recipient, msg );

  if( strlen( recipient ) < 4 )
    return -1;
  
  /* get group */
  len = strlen( recipient ) - 4;
  strncpy( group, recipient + 4, len );
  group[len] = '\0';

  /* get bulletin id */
  bulid = recipient[3];

  /* determine type */
  if( !strncasecmp( recipient, "BLN", 3 )){
    if( isalpha( bulid ))
      type = APRS_ANNOUNCEMENT;
    else if( isdigit( bulid )){
      if( group[0] == '\0' )
	type = APRS_BULLETIN;
      else
	type = APRS_GROUPBULLETIN;
    } else {
      aprslog( -1, "unknown bulletin type in '%s'", recipient );

      return -1;
    }
  } else if( !strncasecmp( recipient, "NWS-", 4 )){
    type = APRS_NWSBULLETIN;
  } else {
    aprslog( -1, "unknown bulletin type in '%s'", recipient );

    return -1;
  }

  if( type == APRS_NWSBULLETIN && !callsign_can_nws( callsign )){
    /* not interested */
    //    return 0;
  }
  
  /* find an existing bulletin */
  if( type != APRS_NWSBULLETIN )
    p = aprs_bul_find( callsign, bulid, group );
  else {
    //    aprslog( -1, "registering bulletin: (%s) (%s) (%s)", callsign, group, msg );
    
    p = aprs_bul_findnws( callsign, group, msg );
  }
  
  if( p == NULL ){
    p = (struct aprs_bulletin *)malloc( sizeof( struct aprs_bulletin ));
    if( p == NULL )
      return -1;

    p->next = NULL;
    p->type = type;
    strcpy( p->callsign, callsign );
    p->bulid = bulid;
    strcpy( p->group, group );
    p->msg[0] = '\0';
    p->msgid[0] = '\0';

    p->expiration = 0;
    p->expiredate[0] = '\0';
    p->category[0] = '\0';
    p->nwsmsg = NULL;
    
    newbul = 1;
  }

  /* set expiration */
  p->expiration = secclock() + aprs_getbulletintimeoutbytype( p->type );

  /* get message */
  c = msg;
  len = 0;
  while( len < 299 && *c != '{' && *c != '\0' ){
    p->msg[len++] = *c++;
  }
  p->msg[len] = '\0';

  /* get message ID */
  while( *c != '{' && *c != '\0' ){
    c++;
  }

  if( *c++ == '{' ){
    for( len = 0; len < 5 && *c != '\0'; len++, c++ )
      p->msgid[len] = *c;
    p->msgid[len] = '\0';
  } else
    p->msgid[0] = '\0';

  /* customizations for NWS bulletins */
  if( newbul && p->type == APRS_NWSBULLETIN ){
    /* get expire date */
    if( p->msg[6] == 'z' )
      aprs_getbulletinnwstimeout( p->msg, p->expiredate );

    /* get category */
    c = p->msg + 8;
    len = 0;
    while( len < 63 && *c != ',' && *c != '\0' ){
      p->category[len++] = *c++;
    }
    p->category[len] = '\0';

    if( *c == ',' )
      c++;

    p->nwsmsg = c;
  }

  if( newbul == 1 ){
    /* add new bulletin to list */

    aprs_bulletinadd( p );
  }
  
  return 0;  
}

void aprs_bulletinadd( struct aprs_bulletin *bul )
{
  struct aprs_bulletin *last, *p;

  p = Aprs_bulletins;
  last = NULL;

  while( p != NULL ){
    if( aprs_bulletincompare( bul, p ) <= 0 )
      break;

    last = p;
    p = p->next;
  }

  if( last == NULL ){
    Aprs_bulletins = bul;
    bul->next = p;
  } else {
    last->next = bul;
    bul->next = p;
  }
     
  return;
}

int aprs_bulletincompare( struct aprs_bulletin *a, struct aprs_bulletin *b )
{
  int ret;
  
  if( (ret = strcmp( a->callsign, b->callsign )) != 0 ){
    if( ret < 0 )
      return -1;
    else
      return 1;
  }

  if( a->type == APRS_GROUPBULLETIN && b->type == APRS_GROUPBULLETIN ){
    if( (ret = strcmp( a->group, b->group )) != 0 ){
      if( ret < 0 )
	return -1;
      else
	return 1;
    }
  }
  
  if( a->bulid != b->bulid ){
    if( a->bulid < b->bulid )
      return -1;
    else
      return 1;
  }

  return 0;
}

struct aprs_bulletin *aprs_bul_find( char *callsign, char bulid, char *group )
{
  struct aprs_bulletin *p;
  
  for( p = Aprs_bulletins; p != NULL; p = p->next )
    if( !strcmp( p->callsign, callsign ) && p->bulid == bulid && !strcmp( p->group, group ))
      return p;

  return NULL;
}

struct aprs_bulletin *aprs_bul_findnws( char *callsign, char *group, char *msg )
{
  struct aprs_bulletin *p;
  char *id;
  int len;
  
  id = strchr( msg, '{' );
  len = (int)(id - msg);
    
  for( p = Aprs_bulletins; p != NULL; p = p->next ){
    //    aprslog( -1, "  Comparing against: (%s) (%s) (%s)", p->callsign, p->group, p->msg );
    if( !strcmp( p->callsign, callsign ) && !strcmp( p->group, group ) && !strncmp( p->msg, msg, len ))
      return p;
  }

  return NULL;
}
					 

void aprs_showbuls( void )
{
  tprintf( "NWS Bulletins\n" );
  aprs_bul_showlist( APRS_NWSBULLETIN );
  tprintf( "\n" );

  tprintf( "Bulletins\n" );
  aprs_bul_showlist( APRS_BULLETIN );
  tprintf( "\n" );

  tprintf( "Announcements\n" );
  aprs_bul_showlist( APRS_ANNOUNCEMENT );
  tprintf( "\n" );

  tprintf( "Group Bulletins\n" );
  aprs_bul_showlist( APRS_GROUPBULLETIN );
  tprintf( "\n" );

  return;
}

void aprs_bul_showlist( int type )
{
  struct aprs_bulletin *p, *plast=NULL;
  char buf[300], ofc[4];
  int len;

  if( type == APRS_NWSBULLETIN )
    tprintf( "Ofc Expires      Category  Affected Counties\n" );
  else if( type == APRS_GROUPBULLETIN )
    tprintf( "Callsign  ID Group Message\n" );
  else
    tprintf( "Callsign  ID Message\n" );
  
  for( p = Aprs_bulletins; p != NULL; ){
    if( !aprs_bul_iscurrent( p )){
      if( plast )
	plast->next = p->next;
      else
	Aprs_bulletins = p->next;

      free( p );

      if( plast )
	p = plast->next;
      else
	p = Aprs_bulletins;

      continue;
    } else if( p->type == type ){
      if( type == APRS_NWSBULLETIN ){
	strncpy( ofc, p->callsign, 3 );
	ofc[3] = '\0';
	
	tprintf( "%-3s %s %-9s ", ofc, p->expiredate, p->category );

	//	len = 299;
	//	aprs_processnwslocations( p->msg, buf, len, 79, 30 );
	tprintf( "%s\n", p->nwsmsg );
      } else if( type == APRS_GROUPBULLETIN )
	tprintf( "%-9s %c  %-5s %s\n", p->callsign, p->bulid, p->group, p->msg );
      else
	tprintf( "%-9s %c  %s\n", p->callsign, p->bulid, p->msg );
    }

    plast = p;
    p = p->next;
  }
    
  return;
}

int nws_addtext( char **p, int *size, int linelen, int indent, int *pos, char *text, char delim )
{
  int len, added=0, d=0, i;

  len = strlen( text );
  if( delim == '.' || delim == ',' ){
    d = 1;
    len++;
  }

 again:
  /* check for room on this line */
  if( linelen - *pos >= len ){
    if( len < *size )
      goto error;
    
    /* add text */
    sprintf( *p, "%s", text );
    *p += len;
    *size -= len;
    *pos += len;
    **p = '\0';

    /* add . or , */
    if( d == 1 ){
      **p = delim;
      *p += 1;
      *size -= 1;
      *pos += 1;
      **p = '\0';
    }
    
    added = 1;
  }

  if( added ){
    /* if there's room on this line, */
    if( linelen - *pos >= 1 + d ){
      /* add spaces after the text */
      for( i = 0; i < d + 1; i++ ){
	if( *size <= 5 )
	  goto error;
	
	**p = ' ';
	*p += 1;
	*size -= 1;
	*pos += 1;
	**p = '\0';
      }
    }
  } else {
    /* go to next line */
    **p = '\n';
    *p += 1;
    *size -= 1;
    *pos = 0;
    **p = '\0';

    /* add indent */
    for( i = 0; i < indent; i++ ){
      if( *size <= 5 )
	goto error;
      
      **p = ' ';
      *p += 1;
      *size -= 1;
      *pos += 1;
      **p = '\0';
    }

    /* try again to add the text */
    goto again;
  }

  return 0;

 error:
  if( *size >= 5 ){
    sprintf( *p, "[...]" );
    *p += 5;
    *size -= 5;
    *pos += 5;
    **p = '\0';
  }

  return -1;
}

int aprs_processnwslocations( char *msg, char *buf, int len, int linelen, int indent )
{
  int num, i, change, ret;
  char lastcode[2], *p;
  int numcounties=0;
  struct county *county, *nextcounty=NULL;
  int pos = indent, size=len - 1;
  p = buf;

  lastcode[1] = lastcode[0] = ' ';

  num = (strlen( msg ) + 1 ) / 7;
  for( i = 0; i < num; i++ ){
    if( nextcounty == NULL )
      county = same_lookupcode( msg + i * 7 );
    else
      county = nextcounty;

    if( i < num - 1 )
      nextcounty = same_lookupcode( msg + i * 7 + 7 );
    else
      nextcounty = NULL;

    /* if next county has a different state */
    if((nextcounty && (county->statecode[0] != nextcounty->statecode[0] || county->statecode[1] != nextcounty->statecode[1] )) ||
       (nextcounty == NULL))
      change = 1;
    else
      change = 0;


    if( numcounties > change ){
      ret = nws_addtext( &p, &size, linelen, indent, &pos, ",", ' ' );
      if( ret == -1 ) return -1;
    }

    if( change && numcounties > 0 ){
      ret = nws_addtext( &p, &size, linelen, indent, &pos, "and", ' ' );
      if( ret == -1 ) return -1;
    }
    
    ret = nws_addtext( &p, &size, linelen, indent, &pos, county->county, ' ' );

    if( change ){
      ret = nws_addtext( &p, &size, linelen, indent, &pos, (numcounties > 0)?"Counties":"County", ' ' );
      if( ret == -1 ) return -1;
      
      ret = nws_addtext( &p, &size, linelen, indent, &pos, "in", ' ' );
      if( ret == -1 ) return -1;

      ret = nws_addtext( &p, &size, linelen, indent, &pos, county->state, '.' );
      if( ret == -1 ) return -1;

      numcounties = 0;
    }
  }

  return 0;
}

int loadstates( void )
{
  FILE *fp;
  char filename[1024];
  char buf[256];
  char *state, *code;
  int count, index;

  States = (char **)malloc( sizeof(char *)*26*26);
  if( States == NULL )
    return -1;

  for( count = 0; count < 26 * 26; count++ )
    States[count] = NULL;

  count = 0;

  sprintf( filename, "%s/states.csv", APRSdir );
  
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return -1;
  while( fgets( buf, 255, fp ) != NULL ){
    strtok( buf, ",\n" );
    state = strtok( NULL, ",\n" );
    code = strtok( NULL, "\n" );

    index = (code[0] - 'A') * 26 + (code[1] - 'A');
    States[index] = j2strdup( state );
    
    count++;
  }

  fclose( fp );

  aprslog( -1, "loaded states" );

  return count;
}

int loadcounties( void )
{
  FILE *fp;
  int count=0;
  char filename[1024];
  char buf[256];
  char *state, *code, *county;

  sprintf( filename, "%s/counties.txt", APRSdir );
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return -1;
  while( fgets( buf, 255, fp ) != NULL ){
    state = strtok( buf, ",\n" );
    code = strtok( NULL, ",\n" );
    county = strtok( NULL, "\n" );


    
  }
  
  fclose( fp );

  aprslog( -1, "loadded counties"  );
  
  return count;
}

int loadzones( void )
{
  FILE *fp;
  int count=0;
  char filename[1024];
  char buf[256];
  char *state, *code, *county;

  sprintf( filename, "%s/zones.txt", APRSdir );
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return -1;
  while( fgets( buf, 255, fp ) != NULL ){
    state = strtok( buf, ",\n" );
    code = strtok( NULL, ",\n" );
    county = strtok( NULL, "\n" );


    
  }
  
  fclose( fp );

  aprslog( -1, "loadded counties"  );
  
  return count;
}

char *aprs_lookupnwscode( char *code )
{
  if( code[2] == 'Z' ){
    /* zone */
  } else if( code[2] == 'C' ){
    /* county */
  }


  return NULL;
}

struct county *ugc_lookupcode( char *code )
{

  return NULL;
}

int loadsamecodes( void )
{
  FILE *fp;
  int count=0, index;
  char buf[256];
  char *code, *county, *state;
  struct county *p;
  char filename[1024];

  SAMEcodes = (struct county **)malloc( sizeof( struct county * ) * 80000 );
  
  sprintf( filename, "%s/SameCode.txt", APRSdir );
  fp = fopen( filename, "r" );
  if( fp == NULL )
    return -1;
  while( fgets( buf, 255, fp ) != NULL ){
    code = strtok( buf, ",\n" );
    county = strtok( NULL, ",\n" );
    state = strtok( NULL, "\n" );

    p = (struct county *)malloc( sizeof( struct county ));
    if( p == NULL )
      return -1;

    p->county = j2strdup( county );
    p->statecode[0] = state[0];
    p->statecode[1] = state[1];
    p->state = States[(state[0]-'A')*26+(state[1]-'A')];

    index = atoi( code );
    SAMEcodes[index] = p;
    
    count++;
  }
  
  fclose( fp );

  aprslog( -1, "loaded SAME codes" );

  return count;
}

struct county *same_lookupcode( char *code )
{
  int n;
  struct county *county;

  n = atoi( code );
  county = SAMEcodes[n];

  return county;
}


int aprs_getbulletintimeoutbytype( int type )
{
  int duration=0;

  switch( type ){
  case APRS_ANNOUNCEMENT:
    duration = announcement_timeout;
    break;
  case APRS_NWSBULLETIN:
    duration = nwsbulletin_timeout;
    break;
  case APRS_BULLETIN:
  case APRS_GROUPBULLETIN:
  default:
    duration = bulletin_timeout;
    break;
  }

  return duration;
}
  
int aprs_bul_iscurrent( struct aprs_bulletin *bul )
{
  if( bul->expiration <= secclock() )
    return 0;
  else
    return 1;
}

int aprs_getbulletinnwstimeout( char *msg, char *datestr )
{
  int day, hour, minute, isdst;
  time_t now, then, diff;
  struct tm *tm;
  
  if( msg[6] != 'z' )
    return 86400;

  day = (msg[0] - '0') * 10 + (msg[1] - '0');
  hour = (msg[2] - '0') * 10 + (msg[3] - '0');
  minute = (msg[4] - '0') * 10 + (msg[5] - '0');
  
  now = time( NULL );
  tm = gmtime( &now );
  if( day < tm->tm_mday )
    tm->tm_mon++;
  if( tm->tm_mon == 12 ){
    tm->tm_mon = 0;
    tm->tm_year++;
  }
  tm->tm_mday = day;
  tm->tm_hour = hour;
  tm->tm_min = minute;
  tm->tm_sec = 0;
  
  isdst = tm->tm_isdst;

  if( datestr ){
    sprintf( datestr, "%2d/%02d %2d:%02dz", tm->tm_mon + 1, day, hour, minute );
  }
  
  then = mktime( tm );
  then += 3600 * (5 - isdst);
  
  diff = then - now;

  return (int)diff;
}

#endif
