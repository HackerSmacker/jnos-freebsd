
#include "global.h"
#ifdef APRSD

/* APRS 
 * 
 * Ack logic
 * 
 * Make sure our transmitted packets get acked.
 */

#include "aprs.h"
#include "timer.h"


struct aprs_ack {
  struct aprs_ack *next;
  
  int rf;
  char *data;
  int len;
  char sender[10];
  char addressee[10];
  char msgno[6];
  int retries;
  time_t nexttx;
};

struct aprs_ack *Aprs_needacks=NULL;
struct timer aprs_acktimer;
// int aprs_maxretries = 9;
int aprs_maxretries = 5;
int aprs_retryintervals[] = { 10, 31, 31, 31, 60, 120, 900, 7200, 43200, 86400 };

extern int aprs_txenable;

void aprs_processack( char *sender, char *addressee, char *msgno );
void aprs_acktick( void *notused );


int aprs_acksetup( void )
{
  aprs_acktimer.func = (void(*)__ARGS((void *)))aprs_acktick;
  aprs_acktimer.arg = NULL;

  return 0;
}


void aprs_ackcheck( char *srccall, char *callsign, char *data )
{
  char msgno[6], *mp;
  int len;

  /* Look for a message ack or reject */
  if( !strncmp( data, "ack", 3 ) || !strncmp( data, "rej", 3 )){

    for( mp = data + 3, len = 0; *mp != '\0' && *mp != '\r' && *mp != '\n' && *mp != '}' && len < 5; mp++, len++ )
      msgno[len] = *mp;
    msgno[len] = '\0';

    aprslog( -1, "aprs_ack: processing ack from %s for %s:%s", srccall, callsign, msgno );
    
    aprs_processack( srccall, callsign, msgno );
  }

  return;
}


/* find the packet that is being acked and remove it from the list */

void aprs_processack( char *sender, char *addressee, char *msgno )
{
  struct aprs_ack *ack, *lastack = NULL;

  for( ack = Aprs_needacks; ack != NULL; lastack = ack, ack = ack->next ){
    aprslog( -1, "aprs_ack: comparing ack addressee (%s) vs packet sender (%s)", addressee, ack->sender );
    if( strcmp( ack->sender, addressee ))
      continue;

    aprslog( -1, "aprs_ack: comparing ack msgno (%s) vs packet msgno (%s)", msgno, ack->msgno );
    if( strcmp( ack->msgno, msgno ))
      continue;

    aprslog( -1, "aprs_ack: comparing ack sender (%s) vs packet addressee (%s)", sender, ack->addressee );
    if( strcmp( ack->addressee, sender ))
      continue;

    /* this ack matches one of our packets */
    aprslog( -1, "aprs_ack: found matching packet.  Removing from list." );
    
    if( lastack == NULL )
      Aprs_needacks = ack->next;
    else
      lastack->next = ack->next;

    ack->next = NULL;
    free( ack->data );
    ack->data = NULL;
    free( ack );
    break;
  }

  return;
}

/* register a packet that needs an ack */

void aprs_needsack( int rf, char *data, int len, char *sender, char *addressee, char *msgno )
{
  struct aprs_ack *ack;
  int left, next;

  /* don't request ack on bulletins */
  if( aprs_isbulletin( addressee ))
    return;
  
  /* first retransmit in 15 seconds */
  next = aprs_retryintervals[0] * 1000;
  
  ack = (struct aprs_ack *)malloc( sizeof( struct aprs_ack ));
  ack->nexttx = secclock() + next / 1000;

  ack->rf = rf;
  ack->data = j2strdup( data );
  ack->len = len;
  
  strncpy( ack->sender, sender, 9 );
  ack->sender[9] = '\0';

  strncpy( ack->addressee, addressee, 9 );
  ack->addressee[9] = '\0';

  strncpy( ack->msgno, msgno, 5 );
  ack->msgno[5] = '\0';

  ack->retries = 0;
  
  /* insert this ack into the needs-acks list */
  ack->next = Aprs_needacks;
  Aprs_needacks = ack;

  /* set the timer for the next ack timeout */
  left = read_timer( &aprs_acktimer );
  if( next < left || left == 0 ){
    aprslog( -1, "aprs_ack: setting timer for %d seconds", next / 1000 );
    stop_timer( &aprs_acktimer );
    set_timer( &aprs_acktimer, next );
    start_timer( &aprs_acktimer );
  }

  aprslog( -1, "aprs_ack: registered packet %s:%s for %s", sender, msgno, addressee );
  
  return;
}

/* retry packets that haven't gotten ack'd yet */

void aprs_acktick( void *notused )
{
  struct aprs_ack *ack, *lastack = NULL; /* 01Jan2024, oops, NULL */
  time_t nexttime=0, now;
  char *p;

  now = secclock();
  
  stop_timer( &aprs_acktimer );
  for( ack = Aprs_needacks; ack != NULL; ack = ack->next ){
    if( ack->nexttx <= now ){
      /* If packet has timed out, remove it from chain.
       * Do this after the timer expires, not when the counter increases, so 
       * that we have a window to receive the ack. 
       */
      if( ack->retries >= aprs_maxretries ){
	if( lastack )
	  lastack->next = ack->next;
	else
	  Aprs_needacks = ack->next;
	
	free( ack->data );
	free( ack );

	if( lastack )
	  ack = lastack;
	else
	  ack = Aprs_needacks;

	/* 16Feb2024, Maiko (VE4KLM), if ack == NULL, then don't continue */ 
	if (!ack)
	{
		aprslog (-1, "prevent the elusive crash - break out of the loop");
		break;
	}
	else aprslog (-1, "aprs_ack %d/%d retries, continue", ack->retries, aprs_maxretries);

	continue;
      }

      /* Retransmit the packet.  Save a copy for next time.  */
      aprslog( -1, "aprs_ack: retrying packet %s:%s for %s", ack->sender, ack->msgno, ack->addressee );

      p = j2strdup( ack->data );

      if( ack->rf ){
	if (sendrf ( p, ack->len ) == -1)
	  aprslog (-1, "sendrf - can't send local message\n");
      } else {
	if (j2send( getHsocket(), p, ack->len, 0 ) < 0 )
	  aprslog (-1, "j2send - can't send igate message\n");
      }
 
      ack->retries++;
      ack->nexttx = now + aprs_retryintervals[ack->retries];
    }

    /* figure out when the timer should go off next */
    if( nexttime == 0 || nexttime > ack->nexttx ){
      nexttime = ack->nexttx;
    }

    lastack = ack;
  }

  if( nexttime ){
    /* set the next tick */
    aprslog( -1, "aprs_ack: setting timer for %d seconds", nexttime - now);
    set_timer( &aprs_acktimer, (nexttime - now) * 1000 );
    start_timer( &aprs_acktimer );
  }
  
  return;
}

void aprs_checkifneedsack( int rf, char *data, int len )
{
  char sender[10], addressee[10], msgno[6], *msg;

  aprslog( -1, "aprs_checkifneedsack: data: %s", data );

  msg = aprs_getsam( data, len, sender, addressee, msgno );
  aprslog( -1, "aprs_checkifneedsack: sender: %s, addressee: %s, msgno: %s", sender, addressee, msgno );
  if( msg )
    aprslog( -1, "aprs_checkifneedsack: msg: %s", msg );

  if( strlen( addressee ) == 0 || strlen( msgno ) == 0 ){
    /* doesn't need to be acked */
    return;
  }

  if( msg && (!strncmp( msg, "ack", 3 ) || !strncmp( msg, "rej", 3 ))){
    /* our ack doesn't need to be acked */
    return;
  }
  
  aprs_needsack( rf, data, len, sender, addressee, msgno );

  return;
}

char *aprs_getsam( char *data, int datalen, char *sender, char *addressee, char *msgno )
{
  char *c, *p, *msg=NULL;
  int len;
  
  c = data;

 again:
  sender[0] = '\0';
  addressee[0] = '\0';
  msgno[0] = '\0';

  /* get srccall */
  p = sender;
  len = 0;
  while( *c && *c != '>' ){
    if( len > 9 ){
      sender[9] = '\0';
      return NULL;
    }

    *p++ = *c++;
    len++;
  }
  *p = '\0';

  /* find end of header */
  while( *c && *c != ':' )
    c++;

  if( *c == '\0' )
    return NULL;

  c++;

  /* check DTI */
  if( *c == '}' )
    goto again;

  if( *c != ':' )
    return NULL;

  c++;

  /* get addressee */
  p = addressee;
  len = 0;
  while( *c && *c != ' ' && *c != ':' ){
    if( len > 9 ){
      addressee[9] = '\0';
      return NULL;
    }

    *p++ = *c++;
    len++;
  }
  *p = '\0';

  while( *c != ':' )
    c++;

  c++;
  msg = c;
  
  /* find msg number */
  while( *c && *c != '{' )
    c++;

  if( *c == '\0' )
    return msg;

  c++;

  p = msgno;
  len = 0;
  while( *c && *c != '}' && *c != '\n' ){
    if( len > 5 ){
      msgno[5] = '\0';
      return msg;
    }

    *p++ = *c++;
    len++;
  }
  *p = '\0';

  return msg;
}

#endif
