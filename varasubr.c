/* low level VARA routines:
 * callsign conversion
 * control block management
 *
 */
#include "global.h"
#ifdef EA5HVK_VARA
#include "mbuf.h"
#include "timer.h"
#include "proc.h"
#include "vara.h"
#include <ctype.h>
#include "ax25.h"

extern char Ttycall[AXALEN];
extern char tncall[AXALEN];
  
struct vara_cb *Vara_cb;
char Varacalls[5][10];

void
vara_cb_closeallbut(iface,vap,reason)
struct iface *iface;
struct vara_cb *vap;
int reason;
{
  struct vara_cb *p;
  int oldstate;
  
  for( p = Vara_cb; p != NULLVARA; p = p->next ){
    /* only on our interface */
    if( p->iface != iface )
      continue;

    /* don't close the listening socket */
    if( p->remote[0] == '\0' )
      continue;
    
    /* don't close the current one */
    if( p == vap )
      continue;

    /* already disconnected */
    if( p->state == VARASTDISC )
      continue;

    oldstate = p->state;
    p->state = VARASTDISC;
    p->reason = reason;
    p->iface = NULLIF;

    if( Vara_debug )
      log (-1, "vara_cb_closeallbut closing cb" );

    if( p->s_upcall != NULLVFP((struct vara_cb*,int,int)))
        (*p->s_upcall)(p,oldstate,VARASTDISC);
  }

  return;
}


/* Look up entry in connection table
 * Check just the interface and direction.  This is different than
 * how the other drivers do it.  This is because VARA can only handle
 * one connection at a time.  Worst case is that we are trying to
 * make an outbound connection (#1) and instead we get an inbound (#2)
 * connection.
 */
struct vara_cb *
find_vara(local,remote,iface,dir)
char *local;
char *remote;
struct iface *iface;
int dir;
{
    register struct vara_cb *vap;
    struct vara_cb *valast = NULLVARA;
    struct vara_cb *vastart;
    
    /* Search list */
    vastart = Vara_cb;

    for(vap = vastart; vap != NULLVARA;valast=vap,vap = vap->next){
      if( vap->iface != iface )
	continue;

      if( vap->conndir != dir )
	continue;

      if( !addreq(remote,vap->remote))
	continue;

      if( valast != NULLVARA ){
	/* Move entry to top of list to speed
	 * future searches
	 */
	valast->next = vap->next;
	vap->next = Vara_cb;
	Vara_cb = vap;
      }
      return vap;
    }

    return NULLVARA;
}
  
  
/* Remove address entry from connection table */
void
del_vara(conn)
struct vara_cb *conn;
{
    register struct vara_cb *vap;
    struct vara_cb *valast = NULLVARA;
  
    if( Vara_debug )
      log (-1, "Entering del_vara" );

    for(vap = Vara_cb; vap != NULLVARA; valast=vap,vap = vap->next){
        if(vap == conn)
            break;
    }
  
    if(vap == NULLVARA)
        return;     /* Not found */
  
    /* Remove from list */
    if(valast != NULLVARA)
        valast->next = vap->next;
    else
        Vara_cb = vap->next;

    /* Free VARA specific things. */
    
    vap->r_upcall = NULLVFP((struct vara_cb*,int));
    vap->s_upcall = NULLVFP((struct vara_cb*,int,int));
    vap->t_upcall = NULLVFP((struct vara_cb*,int));
  
    return;
}

/* Create a VARA control block. Allocate a new structure, if necessary,
 * and fill it with all the defaults.
 */
struct vara_cb *
cr_vara(local,remote,iface,dir)
char *local;
char *remote;
struct iface *iface;
int dir;
{
    struct vara_cb *vap;
#if 0  
    if((remote == NULLCHAR) || (local == NULLCHAR))
        return NULLVARA;
#endif
  
    if( Vara_debug )
      log (-1, "Entering cr_vara" );

    /* Create an entry
     * and insert it at the head of the chain
     */
    vap = (struct vara_cb *)callocw(1,sizeof(struct vara_cb));
    vap->next = Vara_cb;
    Vara_cb = vap;

    vap->txq = NULL;
    vap->rxq = NULL;

    memset( (char *)&vap->flags, 0, 6 );
    
    /*fill in 'defaults'*/
    memcpy(vap->local,local,AXALEN);
    memcpy(vap->remote,remote,AXALEN);
    vap->user = -1;
    vap->jumpstarted = 0;
    vap->state = VARASTDISC;

    vap->conndir = dir;

    if( iface ){
      vap->iface = iface;
      /*      vap->paclen = iface->paclen;*/
      vap->paclen = 1500;
      vap->window = 1500;
    }

    /* Always to a receive and state upcall as default */
    /* Also bung in a default transmit upcall - in case */

    vap->r_upcall = s_vrcall;
    vap->s_upcall = s_vscall;
    vap->t_upcall = s_vtcall;
  
    return vap;
}

/* Get the list of calls to configure VARA with. */

char *
vara_getcalls()
{
  char buf[40];
  char tmp[10];
  char *calls;
  int n=0;
  
  buf[0] = '\0';

  /* Check all of the calls we've configured. 
   * VARA can take up to five callsigns. 
   */
  if( Mycall[0] ){
    strcat( buf, pax25(tmp,Mycall));
    strcpy( Varacalls[n++], tmp );
  }

  if( Bbscall[0] ){
    if( buf[0] ){
      strcat( buf, " " );
    }
    strcat( buf, pax25(tmp,Bbscall));
    strcpy( Varacalls[n++], tmp );
  }

  if( Ttycall[0] ){
    if( buf[0] ){
      strcat( buf, " " );
    }
    strcat( buf, pax25(tmp,Ttycall));
    strcpy( Varacalls[n++], tmp );
  }

  if( tncall[0] ){
    if( buf[0] ){
      strcat( buf, " " );
    }
    strcat( buf, pax25(tmp,tncall));
    strcpy( Varacalls[n++], tmp );
  }

  /* Blank out the ones we don't need. */
  for( ; n<5; n++ )
    Varacalls[n][0] = '\0';

  /* Can't talk to anyone if we didn't find a callsign. */
  if( strlen( buf ) == 0 )
    return NULLCHAR;

  calls = j2strdup( buf );

  return calls;
}

/* Returns true if the call is one of ours.
 * False otherwise.
 */

int
vara_ismycall(call)
char *call;
{
  int i;

  for( i=0; i<5; i++ )
    if( !strcasecmp( Varacalls[i], call ))
      return 1;

  return 0;
}

/* Returns interface with given name */

struct iface *
vara_getifacebyname(name)
char *name;
{
  int i;
  struct ifvara *vif;

  for( i = 0; i < VARA_MAX; i++ ){
    vif = &Vara[i];
    if( vif->iface == NULLIF )
      continue;
    if( !strcmp( name, vif->iface->name ))
      return vif->iface;
  }

  return NULLIF;
}

#endif
