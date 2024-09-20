/* User interface subroutines for VARA
 */
#include <ctype.h>
#include "global.h"
#ifdef EA5HVK_VARA
#include "ax25.h"
#include "vara.h"

struct vara_cb *
open_vara(iface,local,remote,mode,window,r_upcall,t_upcall,s_upcall,user)
struct iface *iface;    /* Interface */
char *local;        /* Local address */
char *remote;       /* Remote address */
int mode;       /* active/passive/server */
int16 window;       /* Window size in bytes */
void (*r_upcall)__ARGS((struct vara_cb *,int));       /* Receiver upcall handler */
void (*t_upcall)__ARGS((struct vara_cb *,int));   /* Transmitter upcall handler */
void (*s_upcall)__ARGS((struct vara_cb *,int,int));   /* State-change upcall handler */
int user;       /* User linkage area */
{
    struct vara_cb *vap;
    char remtmp[AXALEN];
    int conndir, ret;

    if( Vara_debug )
      log( -1, "Entering open_vara" );
    
    if(remote == NULLCHAR){
        remote = remtmp;
        setcall(remote," ");
    }

    if( mode == VA_ACTIVE ){
      conndir = VARA_OUTBOUND;
    } else if( mode == VA_PASSIVE || mode == VA_SERVER ){
      conndir = VARA_INBOUND;
    }
    
    if((vap = find_vara(local,remote,iface,conndir)) != NULLVARA && vap->state != VARASTDISC){
        errno = EISCONN;
        return NULLVARA;    /* Only one to a customer */
    }
    if(vap == NULLVARA && (vap = cr_vara(local,remote,iface,conndir)) == NULLVARA){
        errno = ENOMEM;
        return NULLVARA;
    }

    memcpy(vap->remote,remote,AXALEN);
    memcpy(vap->local,local,AXALEN);

    if(window)  /* If given, use it. Otherwize use interface default */
      vap->window = window;

    vap->r_upcall = r_upcall;
    vap->t_upcall = t_upcall;
    vap->s_upcall = s_upcall;
    vap->user = user;
  
    switch(mode){
        case VA_SERVER:
            vap->flags.clone = 1;
        case VA_PASSIVE:    /* Note fall-thru */
            vap->state = VARASTLISTEN;
	    vara_listen(1);
            return vap;
        case VA_ACTIVE:
            vap->state = VARASTCPEND;
	    ret = vara_connect(vap);

	    if( Vara_debug )
	      log( -1, "vara_connect returned %d", ret );
	    
	    if( ret == -1 ){
	      errno = EBUSY;
	      return NULLVARA;
	    }
            break;
    }

    if( Vara_debug )
      log( -1, "Leaving open_vara" );

    return vap;
}

/* socket level, queue message for sending on txq */
int
send_vara(vap,bp)
struct vara_cb *vap;
struct mbuf *bp;
{
    struct ifvara *vif;

    if( Vara_debug )
      log( -1, "send_vara: %d bytes", bp->cnt );

    
    if(vap == NULLVARA || bp == NULLBUF) {
        free_p(bp);
        return -1;
    }

    if( vap->iface == NULLIF ){
      free_p(bp);
      return -1;
    }

    vif = vap->iface->vara;

    enqueue(&vif->sndq, bp);

    return 0;	/* don't think anybody uses this value */
}

int
va_output(iface,dest,source,pid,data)
struct iface *iface;    /* Interface to use; overrides routing table */
char *dest;     /* Destination AX.25 address (7 bytes, shifted) */
char *source;       /* Source AX.25 address (7 bytes, shifted) */
int16 pid;      /* Protocol ID */
struct mbuf *data;  /* Data field (follows PID) */
{
    /* not written and integrated yet */
#if 0  
    struct mbuf *bp;

    /* Prepend pid to data */
    bp = pushdown(data,1);
    bp->data[0] = (char)pid;
    return vasend(iface,dest,source,LAPB_COMMAND,UI,bp);
#endif

    return -1;
}

/* Socket level: Read message out of receive queue. */

struct mbuf *
recv_vara(vap,cnt)
struct vara_cb *vap;
int16 cnt;
{
    struct mbuf *bp;
  
    if(vap->rxq == NULLBUF)
        return NULLBUF;
  
    if(cnt == 0){
        /* This means we want it all */
        bp = vap->rxq;
        vap->rxq = NULLBUF;
    } else {
        bp = ambufw(cnt);
        bp->cnt = pullup(&vap->rxq,bp->data,cnt);
    }

    if( Vara_debug ){
      log( -1, "recv_vara: Passing mbuf with %d bytes", bp->cnt );
    }
  
    return bp;
}

/* disconnect gracefully */

int
disc_vara(vap)
struct vara_cb *vap;
{
  struct ifvara *vif;

  if( Vara_debug )
    log( -1, "Entering disc_vara" );
  
  if(vap == NULLVARA)
    return -1;

  if( vap->iface == NULL )
    return -1;
  
  vif = &Vara[vap->iface->dev];

  vara_queuecommand( vif, "DISCONNECT" );

  return 0;
}

/* force disconnect */

int
reset_varabyiface(iface)
struct iface *iface;
{
  struct ifvara *vif;

  if( Vara_debug )
    log( -1, "Entering reset_varabyiface" );

  if( iface == NULLIF )
    return -1;

  vif = iface->vara;

  vara_queuecommand( vif, "ABORT" );

  return 0;
}

int
reset_vara(vap)
struct vara_cb *vap;
{
  struct ifvara *vif;

  if( Vara_debug )
    log( -1, "Entering reset_vara" );

  if(vap == NULLVARA)
    return -1;

  vif = vap->iface->vara;

  vara_queuecommand( vif, "ABORT" );

  return 0;
}

/* do retransmit */

int
kick_vara(vap)
struct vara_cb *vap;
{
	register struct vara_cb *vapd;

	if( Vara_debug )
	  log( -1, "Entering kick_vara" );

  if (vap != NULLVARA)
	{
	   	for (vapd = Vara_cb; vapd != NULLVARA; vapd = vapd->next)
		{
       		if (vap == vapd)
			{
				recover (vap);
				break;
			}
		}
	}

	return 0;
}

/* used by PPP code */
int
get_vara(dev)
int dev;
{
    struct ifvara *vif;

    vif = &Vara[dev];
    if (vif->iface == NULLIF)
	return -1;
    while (!vif->rcvq)
    {
	if (pwait(&vif->rcvq) != 0)
	    return -1;		/* may not be dead, e.g. alarm in dialer */
    }
    return PULLCHAR(&vif->rcvq);
}

/* Send a message on the specified vara interface */
/* used by PPP code */
int
vara_sendbuf(dev,bp)
int dev;
struct mbuf *bp;
{
  struct ifvara *vif;

  if(dev < 0 || dev >= VARA_MAX || (vif = &Vara[dev])->iface == NULLIF){
    free_p(bp);
    return -1;
  }
  enqueue(&vif->sndq, bp);
  return 0;
}

struct vara_cb *getvaracb (char *vacbid)
{
    register struct vara_cb *vapd, *vap = NULLVARA;

	char vararcall[AXALEN];
 
	if (isdigit (*vacbid)) 
	{
	    /* 09Mar20121, Maiko (VE4KLM), replace htoi with htol, and better way anyways */
	  vapd = (void*)htol (vacbid);

    	if (vapd != NULLVARA)
		{
	    	for (vap = Vara_cb; vap != NULLVARA; vap = vap->next)
        		if (vap == vapd)
					break;
		}
	}
	else
	{
		setcall (vararcall, vacbid);

		for (vap = Vara_cb; vap != NULLVARA; vap = vap->next)
		  if (addreq (vap->remote, vararcall))
				break;
	}

	return vap;
}

#endif
