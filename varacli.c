/*
 * Support for EA5HVK VARA (tcp/ip control) as a digital modem
 *
 * January 5, 2021, The beginnings of a prototype interface.
 *
 * January 16, 2021, Now designed as an IP Bridge using PPP code :)
 *
 * January 28, 2021, Added remote RTS PTT control using UDP, use
 * it in conjunction with the new udpPTT.txt (perl) script, note
 * the script is hardcoded for COM1, edit if you need too ...
 *
 * September 1, 2021, Looks like access mode is finally working to
 * a reasonable degree - did 4 kicks in a row to a winlink express
 * running VARA HF P2P mode, 2 empty, 1 message pickup, followed by
 * another empty, seems to be working, and smooth for a change.
 *  (using full blast licensed VARA - a big thank you to Jose)
 *
 * AND there is now a global flag 'vara_ip_bridge_mode' which
 * will be 0 (off) by default, such that vara access mode will
 * be the default behaviour. You must toggle this flag using
 * the new 'vara ipbridge' before using it with PPP ...
 *
 * The PPP over VARA is working reasonably well, latency is not
 * great, but to be honest, PPP, IP, any stop and start traffic
 * where you're prompted for login and password, these scenarios
 * aren't really suitable for VARA - but it certainly works !
 *
 * (C)opyright 2021 Maiko Langelaar, VE4KLM
 *
 * For Amateur Radio use only (please) !
 *
 */

#include "global.h"

#ifdef	EA5HVK_VARA
#include "vara.h"
#include "netuser.h"
#include "ax25.h"
#include "pktdrvr.h"

struct ifvara Vara[VARA_MAX];
extern int Vara_connectiontimeout;

int vara_attach (int argc, char *argv[], void *p)
{
  struct iface *ifp;
  struct varaconn *vcp;
  struct ifvara *vap;
  int dev;
  int len;

  if(( len = sizeof(struct sockaddr_va)) != MAXSOCKSIZE ){
    log(-1, "sockaddr_va is wrong size: %d", len );
    return -1;
  }
  
  /* See if there's already an interface by the same name. */
	if (if_lookup (argv[1]) != NULLIF)
	{
		tprintf (Existingiface, argv[1]);
		return -1;
	}

	/* Create interface structure and fill in details */

	ifp = (struct iface*)callocw (1, sizeof(struct iface));

	ifp->addr = Ip_addr;
	ifp->name = j2strdup (argv[1]);

	setencap( ifp, "VARA" );

	ifp->hwaddr = mallocw(AXALEN);
        memcpy(ifp->hwaddr,Mycall,AXALEN);

        ifp->mtu = atoi(argv[2]);

	ifp->flags = 0;
	ifp->xdev = 0;

	ifp->stop = vara_stop;
	ifp->ioctl = vara_ioctl;
	ifp->show = vara_show;
	
	/* Create structure for VARA info */
	for(dev=0;dev < VARA_MAX;dev++){
	  vap = &Vara[dev];
	  if(vap->iface == NULLIF)
	    break;
	}
	vap->iface = ifp;
	ifp->vara = vap;
	
	if(dev >= VARA_MAX){
	  j2tputs("Too many VARA modems\n");
	  return -1;
	}
	ifp->dev = dev;
	vap->ready = 0;

	/* 
	 * Get the modem connection endpoints. 
	 * There are three, with default ports:
	 * Port 8300 - Command channel
	 * Port 8301 - Data channel
	 * Port 8100 - KISS channel for broadcasts
	 * We'll set the first two on attach, and the third one
	 * later if needed.
	 */

	/* First the command channel */
	vcp = (struct varaconn *)callocw(1, sizeof(struct varaconn));
	if( vcp == NULL ){
	  tprintf("Out of memory");
	  return -1;
	}

	vcp->hostname = j2strdup (argv[3]);
	vcp->port = atoi (argv[4]);
	vcp->s = -1;

	vap->command = vcp;

        vap->cmdq = NULL;

	
	/* Then the data channel */
	vcp = (struct varaconn *)callocw(1, sizeof(struct varaconn));
	if( vcp == NULL ){
	  tprintf("Out of memory");
	  return -1;
	}

	vcp->hostname = j2strdup (argv[3]);
	vcp->port = atoi (argv[4]) + 1;
	vcp->s = -1;

	vap->comms = vcp;

	/* Then add the PTT channel */
	vcp = (struct varaconn *)callocw(1, sizeof(struct varaconn));
	if( vcp == NULL ){
	  tprintf("Out of memory");
	  return -1;
	}

	vcp->hostname = NULL;
	vcp->port = 0;
	vcp->s = -1;

	vap->ptt = vcp;

#ifdef _4Z1AC_VARAC
	/* Save broadcasts for later */
	vap->kiss = NULL;
#endif

	/* Link in the interface - important part !!! */
	ifp->next = Ifaces;
	Ifaces = ifp;

	/* Init the interface */
	vara_initinterface(dev,ifp,vap);

	/*
	 * Command Processor
	 */

	ifp->supv = newproc ("vara_cmd", 1024, vara_supv, dev, (void*)ifp, (void*)vap, 0);

 	/*
	 * Data (Rx) Processor
 	 */

	ifp->rxproc = newproc ("vara_rx", 1024, vara_rx, dev, (void*)ifp, (void*)vap, 0);

	/*
	 * Data (Tx) Processor
 	 */

	ifp->txproc = newproc ("vara_tx", 1024, vara_tx, dev, (void*)ifp, (void*)vap, 0);

	return 0;
}

int vara_initinterface(dev,ifp,vif)
int dev;
struct iface *ifp;
struct ifvara *vif;
{
  vif->ptttype = VARA_PTTVOX;

  vif->cmdq = NULL;
  vif->sndq = NULL;
  vif->rcvq = NULL;
  vif->outbw = j2strdup( "BW500" );
  vif->inbw = j2strdup( "BW500" );
  vif->listen = 0;
  vif->busy = 0;
  vif->carrier = 0;
  vif->direction = 0;
  vif->starttime = 0;
  vif->bandwidth = NULLCHAR;
  vif->txbytes = 0;
  vif->txbuf = 0;
  vif->rxbytes = 0;
  vif->busytime = 0;

  vif->t1.func = vara_connecttimedout;
  vif->t1.arg = vif;
  set_timer(&vif->t1,Vara_connectiontimeout*1000);

  vif->t2.func = vara_connectionrampup;
  vif->t2.arg = vif;
  set_timer(&vif->t2,Vara_startuptimeout*1000);

  vif->t3.func = vara_sessiontimedout;
  vif->t3.arg = vif;
  set_timer(&vif->t3,Vara_sessiontimeout*1000);

  vif->attachtime = secclock();
  vif->busystart = 0;
  vif->busytime = 0;

  vif->txactive = 0;
  vif->txq = 1;
  vif->txints = 0;
  vif->txget = 0;
  vif->txblock = 0;
  vif->txovq = 0;
  
  vif->didreset = 0;
  vif->ready = 0;

  return 0;
}

int vara_initmodem(vif)
struct ifvara *vif;
{
  char *calls;
  char buf[100];

  /* Kill any connection after a loss of heartbeat. */
  if( vif->didreset ){
    vara_queuecommand( vif, "ABORT" );
    vif->didreset = 0;
  }

  vif->txactive = 0;
  
  /* get version for logs */
  vara_queuecommand( vif, "VERSION" );

  /* set calls */
  calls = vara_getcalls();
  sprintf( buf, "MYCALL %s", calls );
  vara_queuecommand( vif, buf );

  /* set bandwidth */
  vara_queuecommand( vif, vif->outbw );

  /* set compression */
  vara_queuecommand( vif, "COMPRESSION ON" );
  
  return 0;
}

int
vara_stop(iface)
struct iface *iface;
{
  struct ifvara *vif;

  vif = &Vara[iface->dev];

  /* Timers should already be stopped, but just in case... */
  stop_timer( &vif->heartbeat );
  stop_timer( &vif->txwatchdog );
  stop_timer( &vif->t1 );
  stop_timer( &vif->t2 );
  stop_timer( &vif->t3 );

  
  vif->iface = NULLIF;
  vif->ready = 0;

  /* free all memory */

  if( vif->command ){
    free( vif->command->hostname );
    free( vif->command );
  }

  if( vif->comms ){
    free( vif->comms->hostname );
    free( vif->comms );
  }

  if( vif->ptt ){
    free( vif->ptt->hostname );
    free( vif->ptt );
  }
  
  free( vif->outbw );
  free( vif->inbw );
  free( vif->bandwidth );

  iface->vara = NULL;
  
  return 0;
}

void vara_flushcommandqueue(vif)
struct ifvara *vif;
{
  struct varacmd *p, *next;

  if( Vara_debug )
    log( -1, "Entering vara_flushcommandqueue" );

  for( p = vif->cmdq; p; p = next ){
    next = p->next;

    free( p->command );
    free( p );
  }

  vif->cmdq = NULL;

  return;
}
			   

int
vara_queuecommand(vif,command)
struct ifvara *vif;
char *command;
{
  struct varacmd *p, *node;
  int qlen=2;

  if( command == NULL )
    return -1;

  if( !vif->ready ){
    if( Vara_cmddebug )
      log( -1, "vara_queuecommand: interface not ready" );
    return -1;
  }

  p = (struct varacmd *)malloc( sizeof( struct varacmd ));
  p->command = j2strdup( command );
  strupr( p->command );
  p->next = NULL;

  if( vif->cmdq == NULL ){
    vif->cmdq = p;

    if( Vara_cmddebug )
      log( -1, "vara cmd queuelen: 1" );
    
    return 0;
  }

  for( node = vif->cmdq; node->next; node = node->next )
    qlen++;
  node->next = p;

  if( Vara_cmddebug )
    log( -1, "vara cmd queuelen: %d", qlen );

  return 0;
}

char *
vara_dequeuecommand( vif )
struct ifvara *vif;
{
  char *command;
  struct varacmd *p;
  
  if( vif->cmdq == NULL ){
    return NULL;
  }

  p = vif->cmdq;
  vif->cmdq = p->next;

  command = p->command;
  free( p );

  return command;
}

void
vara_connecttimedout(p)
void *p;
{
  struct ifvara *vif=(struct ifvara *)p;

  if( Vara_debug )
    log(-1,"vara: connection attempt timed out.  Closing things up");
  
  /* stop the connect attempt */
  vara_queuecommand(vif,"ABORT");

  /* send a close to the socket */
  vara_recvendconn(vif,VA_TIMEOUT);
  
  return;
}

void
vara_sessiontimedout(p)
void *p;
{
  struct ifvara *vif=(struct ifvara *)p;

  if( Vara_debug )
    log(-1,"vara: session timed out.  Closing things up");
  
  /* end the connection */
  vara_queuecommand(vif,"ABORT");

  /* send a close to the socket */
  vara_recvendconn(vif,VA_TIMEOUT);
  
  return;
}

void
vara_connectionrampup(p)
void *p;
{
  struct ifvara *vif=(struct ifvara *)p;

  stop_timer( &vif->t2 );
  
  /* 
   * See if we've actually transmitted enough data to show
   * the connection is succeeding and not stuck in a 
   * negotiation loop. 
   */
  if( vif->txbytes - vif->txbuf < Vara_startupbytes ){
    if( Vara_debug )
      log(-1,"vara: connection didn't progress.  Closing things up");
  
    /* stop the connect attempt */
    vara_queuecommand(vif,"ABORT");

    /* send a close to the socket */
    vara_recvendconn(vif,VA_TIMEOUT);
  } else {
    if( Vara_debug )
      log(-1,"vara: connection progressed successfully");
  }
  
  return;
}

#endif
