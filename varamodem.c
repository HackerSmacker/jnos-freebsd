/*
 * VARA modem
 */

#include "global.h"

#ifdef	EA5HVK_VARA
#include <unistd.h>
#include "socket.h"
#include "netuser.h"
#include "vara.h"
#include "ax25.h"
#include "devparam.h"
#include "pktdrvr.h"
#include "main.h"

void vara_recvnewconn(struct ifvara *,struct varahdr *);
int vara_commandloop(struct ifvara *,int);
extern int j2directsource (int socknum, char *filename);
int vara_sendstream(int,struct iface *,struct mbuf *);


/* Listen state
 * Save this state so that if another interface comes up, we can move it into listen.
 * Applies to all VARA interfaces 
 */
int Vara_listening=0;

/* Startup timeout to avoid infinite loop in poor signal conditions.
 * After connect, requires X bytes to be transmitted in the first Y seconds. 
 * Default to 0 (disabled).  Sysop can adjust as necessary.
 */
int Vara_startuptimeout=60;
int Vara_startupbytes=0;

/* Transmit watchdog timer.  Set PTT off if transmitter is stuck. */
int Vara_transmittimeout=60;

/* Max length of VARA session before disconnecting. */
int Vara_sessiontimeout=900;

/* Max length for VARA connection to become established. */
int Vara_connectiontimeout=60;

/* Enable broadcast to support */
int Vara_broadcast=0;

/* Flag to see VARA driver debugging output. */
int Vara_debug=0;

/* Flag to see all of the commands and responses from the VARA modem. */
int Vara_cmddebug=0;


int
vara_ipconnect(hostname, port, mode)
char *hostname;
int port;
int mode;
{
  struct sockaddr_in fsocket;
  int s;
  static int logstage=1, logcount=0;

  if ((fsocket.sin_addr.s_addr = resolve (hostname)) == 0L)
      {
	log (-1, "vara - host (%s) not found", hostname);
        return -1;
      }
  
  fsocket.sin_family = AF_INET;
  fsocket.sin_port = port;
    
  if ((s = j2socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      log (-1, "vara - no socket");
      return -1;
    }

  if (j2connect (s, (char*)&fsocket, SOCKSIZE) == -1)
    {
      /*
       * 04May2021, Maiko (VE4KLM), Thanks to Bob (VE3TOK) who noticed a
       * growing amount of winrpr_rx sockets - way too many of them ! So
       * it's important to close the socket if we are unable to connect,
       * same applies to agwpe and this new vara code for that matter.
       */
      close_s (s);
      s = -1;
      
      /*
       * 26Nov2020, Maiko (VE4KLM), an idea to back off on the number of
       * log entries created if the device is offline for long periods.
       */
      if (!(logcount % logstage))
	{
	  log (-1, "vara [%s:%d] - connect failed, errno %d", hostname, port, errno);
	  logstage *= 2;
	  logcount = 0;
	}
      
      logcount++;
    }
  else
    {
      logcount = 0;
      logstage = 1;
    }

  if( s != -1 && mode == VARA_TEXT ){
    sockmode (s, SOCK_ASCII);
    seteol (s, "\r");	/* Windows VARA HF v4.3.1 requires this */
  }

  if( Vara_debug )
    if( s != -1 )
      log (-1, "vara [%s:%d] - made connection", hostname, port);
  
  return s;
}

/* 
 * Make sure the command line to the modem is still open.  If we get
 * here, we missed a heartbeat. 
 */

void
vara_heartbeat(arg)
void *arg;
{
  struct ifvara *vif=(struct ifvara *)arg;

  log(-1,"*** vara lost heartbeat");

  /* Ensure we unkey the transmitter */
  varaptt( vif, "OFF" );

  /* kill the TCP connections to the modem and reopen them */
  if( vif->command->s != -1 ){
    close_s( vif->command->s );
    vif->command->s = -1;
  }

  if( vif->comms->s != -1 ){
    close_s( vif->comms->s );
    vif->command->s = -1;
  }

  /* close any open sockets */
  vara_recvendconn( vif, VA_IOERROR );

  /* stop the connection timer */
  stop_timer( &vif->t1 );
  
  /* stop the progression timer */
  stop_timer( &vif->t2 );
  
  /* stop the session timer */
  stop_timer( &vif->t3 );
  
  /* close any open connection on the modem */
  vif->didreset = 1;

  return;
}

/* 
 * Be careful that we don't hit PTT and it stays on too long.
 * If we got here, transmit might be stuck.
 */

void
vara_txwatchdog(arg)
void *arg;
{
  struct ifvara *vif=(struct ifvara *)arg;

  log(-1,"*** vara_txwatchdog fired");
  
  varaptt( vif, "OFF" );

  return;
}

/* 
 * Supervisor thread *
 */

void
vara_supv(dev,p1,p2)
int dev;
void *p1, *p2;
{
  struct ifvara *vap;
  
  vap = &Vara[dev];

  /* Set heartbeat timer.
   * Should receive a keepalive from the modem every 60 seconds.
   * If we get to 75, something is wrong.
   */

  vap->heartbeat.func = (void(*)__ARGS((void *)))vara_heartbeat;
  vap->heartbeat.arg = (void *)vap;
  set_timer( &vap->heartbeat, 75000 );

  /* Set txwatchdog timer.
   * Need to make sure we don't transmit for too long.
   * US legal timeout for stuck transmitter is 3 minutes.  But with 
   * VARA, 30 seconds would probably still be too long.
   */

  vap->txwatchdog.func = (void(*)__ARGS((void *)))vara_txwatchdog;
  vap->txwatchdog.arg = (void *)vap;


  while(!main_exit){
  
    /* open connection to control port */
    vap->command->s = vara_ipconnect( vap->command->hostname, vap->command->port, VARA_TEXT );
    if( vap->command->s == -1 ){
      j2pause( 10000 );
      continue;
    }

    start_timer( &vap->heartbeat );

    vap->ready = 1;
    vara_initmodem( vap );
    
    /* if 'vara start' has already been run, catch this interface up */
    if( Vara_listening ){
      vara_queuecommand( vap, "LISTEN ON" );
    }

    vara_commandloop( vap, vap->command->s );

    stop_timer( &vap->heartbeat );
    vap->ready = 0;

    vara_flushcommandqueue( vap );
    close_s( vap->command->s );
    vap->command->s = -1;
  }

  /* process is ending */
  return;
}    


/* 
 * As long as the command socket is open, look for commands and
 * send them, and also look for responses and process them.
 */

int
vara_commandloop(vif,s)
struct ifvara *vif;
int s;
{
  char buf[100], *command;
  int len, ret;
  int save_errno;
  struct varahdr hdr;
  
  while(!main_exit){
    j2alarm (250);
    len = recvline (s, buf, sizeof(buf) - 1);
    save_errno = errno;
    j2alarm(0);

    /* Read responses from control port */
    if (len > 0)
      {
	rip (buf);

      if (Vara_cmddebug)
	log (s, "vara [%s]", buf);

      /*
       * VARA RESPONSES
       */
      
      if (!strncmp (buf, "PTT ON", 6)){
	varaptt( vif, "ON" );

      } else if (!strncmp (buf, "PTT OFF", 7)){
	varaptt( vif, "OFF" );

      } else if (!strncmp (buf, "BUFFER", 6)){
	/* 29May2021, Maiko (VE4KLM), Forgot Jose mentioned to me this back
	 * in January I think, it's an important number to keep an eye on,
	 * and since I'm stuck on the lzhuf portion of forwarding, perhaps
	 * this needs to be incorporated in the lzhuf portion ? We'll see.
	 *  (presently I am NOT actually using it - 01Sep2021, Maiko)
	 */
	vif->txbuf = atoi (buf+7);
	if( vif->txbuf > 0 )
	  vif->txactive = 1;
	else
	  vif->txactive = 0;

      } else if (!strncmp (buf, "CONNECTED", 9)){

	/* cancel the connect timeout timer */
	stop_timer(&vif->t1);

	/* flush any data in the rcv queue */
	free_q( &vif->rcvq );
	vif->rcvq = NULLBUF;
	
	/* if outgoing call started, mark busy */
	/* if incoming call started, already busy, but doesn't hurt to mark busy again */
	vif->busy = 1;
	if( !vif->busystart )
	  vif->busystart = secclock();

	/* have an active connection */
	vif->carrier = 1;

	/* mark time the connection started */
	vif->starttime = secclock();

	/* reset connection statistics */
	vif->txbytes = 0;
	vif->txbuf = 0;
	vif->rxbytes = 0;

	/* start the progression timer */
	if( Vara_startuptimeout > 0 && Vara_startupbytes > 0 ){
	  set_timer( &vif->t2, Vara_startuptimeout * 1000 );
	  start_timer( &vif->t2 );
	}
	
	/* start the session timer */
	if( Vara_sessiontimeout > 0 ){
	  set_timer( &vif->t3, Vara_sessiontimeout * 1000 );
	  start_timer( &vif->t3 );
	}

	/*
	 * 16Jan2021, Maiko (VE4KLM), I should probably indicate the
	 * physical layer is UP when we are actually connected, not
	 * when the data stream is connected, elsewhere in the code.
	 *
	 * As well, source the ppp commands, so that I don't have to
	 * manually run a 'source ppp.nos' after I see the connect.
	 *
	 * Lastly, indicate physical layer is DOWN on disconnect.
	 *
	 */
	char *cp, *src, *dest;
	
	src = buf + 10;
	
	cp = src;
	while( *cp != ' ' )
	  cp++;
	*cp = '\0';
	cp++;
	dest = cp;
	
	while( *cp != ' ' )
	  cp++;
	*cp = '\0';
	

	if( vara_ismycall( dest ))
	  {
	    /* Inbound call */
	    vif->direction = hdr.conndir = VARA_INBOUND;
	    
	    log( -1, "vara: received connection from %s", src );

	  }
	else
	  {
	    /* Outbound call */
	    vif->direction = hdr.conndir = VARA_OUTBOUND;
	    
	    log( -1, "vara: connected to %s", dest );
	    
	  }

	setcall( vif->srcaddr, src );
	setcall( vif->dstaddr, dest );

	setcall( hdr.srcaddr, src );
	setcall( hdr.dstaddr, dest );

	/* TODO: Add in calls for digi's if they're there. */
	
	vara_recvnewconn( vif, &hdr );

	/* 
	 * 01Sep2021, Maiko (VE4KLM), no more hard coding, one can now configure
	 * the VARA interface as an IP Bridge (PPP), or to simply use for direct
	 * access for forwarding of messages with other VARA based BBS systems.
	 */
	if ( vif->iface->type == CL_PPP )
	  {
	    log (s, "physical layer (PPP) is up");
	    
	    /* 15Jan2021, Maiko (VE4KLM), you gotta be kidding me :| */
	    if (vif->iface->iostatus) (*vif->iface->iostatus)(vif->iface, PARAM_UP, 0);
	    
	    log (s, "sourcing PPP configuration");
	    
	    /* Maiko, new breakout function in main.c */
	    j2directsource (s, "ppp.nos");
	  }

      } else if (!strncmp (buf, "DISCONNECTED", 12)){
	/* outgoing call or incoming call just ended */
	/* mark unbusy */
	vif->busy = 0;
	if( vif->busystart ){
	  vif->busytime += secclock() - vif->busystart;
	  vif->busystart = 0;
	}

	/* stop the connection timer */
	stop_timer( &vif->t1 );
	
	/* stop the progression timer */
	stop_timer( &vif->t2 );

	/* stop the session timer */
	stop_timer( &vif->t3 );

	/* no active connection */
	vif->carrier = 0;

	/* send up notice of connection end */
	vara_recvendconn( vif, VA_NORMAL );

	/* 
	 * 01Sep2021, Maiko (VE4KLM), no more hard coding, one can now configure
	 * the VARA interface as an IP Bridge (PPP), or to simply use for direct
	 * access for forwarding of messages with other VARA based BBS systems.
	 */
	if ( vif->iface->type == CL_PPP )
	  {
	    log (s, "physical layer (PPP) is down");
	    
	    if (vif->iface->iostatus) (*vif->iface->iostatus)(vif->iface, PARAM_DOWN, 0);
	  }

      } else if( !strncmp( buf, "IAMALIVE", 8 )){
	/* Reset watchdog timer */
	start_timer( &vif->heartbeat );
	
      } else if( !strncmp( buf, "OK", 2 )){
      } else if( !strncmp( buf, "WRONG", 5 )){
      } else if( !strncmp( buf, "REGISTERED", 10 )){
      } else if( !strncmp( buf, "LINK REGISTERED", 15 )){
      } else if( !strncmp( buf, "TUNE", 4 )){
      } else if( !strncmp( buf, "BUSY ON", 7 )){
      } else if( !strncmp( buf, "BUSY OFF", 8 )){
      } else if( !strncmp( buf, "PENDING", 7 )){
	/* possible incoming connection */
	/* mark busy */
	vif->busy = 1;
	vif->busystart = secclock();
	
      } else if( !strncmp( buf, "CANCELPENDING", 13 )){
	/* incoming call hung up before we answered */
	/* mark unbusy */
	vif->busy = 0;
	if( vif->busystart ){
	  vif->busytime += secclock() - vif->busystart;
	  vif->busystart = 0;
	}
	
      } else if( !strncmp( buf, "SN", 2 )){
	/* Signal to Noise ratio */
      }
    }
  else if (len == -1)
    {
      if (save_errno != EALARM)
	{
	  log (s, "vara command disconnected, errno %d", save_errno);

	  if( Vara_debug )
	    log (s, "resetting vara modem network interfaces");	/* 01Sep2021, Maiko, more informative */
	  
	  return -1;
	}
    }

    if( s == -1 ){
      if( Vara_debug )
	log( -1, "vara_commandloop: s == -1" );
    }
    
  /* send commands to control port */
    if (vif->cmdq != NULL && s != -1 ) /* An opportunity to issue a command */
	{
	  while((command = vara_dequeuecommand(vif))!=NULL){
	    if( Vara_cmddebug )
	      log( -1, "vara_commandloop: sending command \"%s\"", command );

	    if( !strcmp( command, "RESETINTERFACE" )){
	      /*
	       * 01Feb2021, Maiko (VE4KLM), Strange behavior more so on windows
	       * 10, give user option to reset entire TCP/IP interface, saves from
	       * having to restart JNOS, I don't like having to add this command,
	       * but it's the only way right now to 'reset stuff', sigh ...
	       */

	      /* shut it down and let it come back up */
	      close_s( vif->command->s );
	      vif->command->s = -1;
	      close_s( vif->comms->s );
	      vif->comms->s = -1;
	      
	    } else {
	      ret = j2send (s, command, strlen (command), 0);
	      if( ret == -1 ){
		close_s( vif->command->s );
		vif->command->s = -1;
	      }

	      ret = j2send (s, "\r", 1, 0);
	      if( ret == -1 ){
		close_s( vif->command->s );
		vif->command->s = -1;
	      }
	    }
	    free (command);
	  }
	}

    if( save_errno == EALARM )
      j2pause( 250 );
  } /* while */

  return 0;
}

#ifdef _4Z1AC_VARAC

kissthread()
{
  /* open connection to kiss port */
  /* receive kiss frames on port */
}

#endif


/* 
 * The process reading data off the queue and sending it to the modem. 
 * tx on commport for stream connections 
 * tx on kissport for broadcasts
 */

void
vara_tx(dev, p1, p2)
int dev;
void *p1, *p2;
{
    register struct mbuf *bp;
    struct ifvara *vif;
    struct iface *iface = (struct iface *)p1;
    int c, ret;

    vif = &Vara[dev];

    /* 
     * will need to split this function in two once we start handling the kiss
     * port.  Some will go as a TCP stream to the modem, other as KISS.
     */

    if ((c = vif->txq) <= 0)
      c = 1;
    
    while(!main_exit){
	while (vif->sndq == NULLBUF)
	{
	    if (!(c = vif->txq))
	      c = 1;
	    if (pwait(&vif->sndq) != 0) {
	      log(-1, "vara_tx: pwait failed with errno %d", errno );
	      iface->txproc = NULLPROC;
	      return;
            }
	    vif->txints++;
	}
	bp = dequeue(&vif->sndq);

	if( Vara_debug )
	  log(-1,"vara_tx: dequeued mbuf with %d bytes", bp->cnt );
	vif->txget++;

	/* 
	 * Handle stream data. 
	 */
	
	/* If we're not connected, we should just let the buffer flush. */
	if( !vif->carrier ){
	  if( Vara_debug ){
	    log(-1,"vara_tx: No carrier, dropping data");
	  }
	  free_mbuf(bp);
	  bp = NULLBUF;
	  continue;
	}

	/* Transmit the data */
	ret = vara_sendstream( dev, iface, bp );
	
	if (--c < 1 && vif->txq > 1)
	{
	    vif->txovq++;
	    pwait(NULL);
	}
	
    }

    log( -1, "vara_tx exiting");
    
    return;
}

int
vara_sendstream(dev,iface,bp)
int dev;
struct iface *iface;
struct mbuf *bp;
{
  struct ifvara *vif;
  int cnt, l, b=0;

  vif = &Vara[dev];

  if( bp == NULLBUF )
    return 0;

  cnt = bp->cnt;
  l = send_mbuf( vif->comms->s, bp, 0, NULL, 0 );
  bp = NULLBUF;

  if( l >= 0 && l != cnt ){
    log(-1,"vara_tx: sent %d vs %d", l, cnt );
  }
  
  if( Vara_debug ){
    if( l == -1 )
      log(-1,"vara_tx: couldn't write to channel, errno=%d", l, errno );
    else
      log(-1,"vara_tx: wrote %d bytes to channel", l, errno );
  }
  
  if (l == -1) {  /* some write error */
    switch( errno ){
    case EBADF:
    case ENOTCONN:
      return -1;
    case EWOULDBLOCK:
      l = cnt;
      b = 1;
      break;
    default:
      log(vif->comms->s,"vara_sendstream: unknown error %d",errno);
    }
  }
  
  if( l > 0 ){
#ifdef J2_SNMPD_VARS
    iface->rawsndcnt++;
    iface->lastsent = secclock();
    iface->rawsndbytes += l;
#endif
    vif->txbytes += l;
    vif->txbuf += l;
    vif->txactive = 1;
	
    if( Vara_cmddebug ){
      /* Using cmddebug because the main buffer messages come from the command stream. */
      log( vif->comms->s, "vara [BUFFER %d]*", vif->txbuf );
    }
  }

  if( b ){
    vif->txblock++;
    pwait(NULL);
  }
  
  return 0;
}

void
vara_input(dev, arg1, arg2)
    int dev;
    void *arg1, *arg2;
{
#if 0
    extern int errno;
    struct iface *ifp = (struct iface *) arg1;
    fd_set fds;
#endif

    /* not written */

    return;
}

/*
 * open connection to commport;
 * receive text and send up;
 */

void
vara_rx(dev,p1,p2)
int dev;
void *p1;
void *p2;
{
#if 0
    struct iface *ifp = p1;
#endif
    struct mbuf *bp;
    struct iface *iface;
    struct ifvara *vif;
    int s, cnt;
    struct sockaddr_va from;
    int fromlen;
    
    vif = &Vara[dev];
    iface = vif->iface;
    
    while(!main_exit){
      /* open connection to data port */
      s = vif->comms->s = vara_ipconnect( vif->comms->hostname, vif->comms->port, VARA_TEXT );
      if( s == -1 ){
	j2pause( 10000 );
	continue;
      }

      if( Vara_debug )
	log (-1, "vara data rx established [%s:%d]", vif->comms->hostname, vif->comms->port );

      sockmode( s, SOCK_ASCII );
      seteol (s, "\r");	/* Windows VARA HF v4.3.1 requires this */
      
      /* receive data */
      while(!main_exit){
	cnt = recv_mbuf( vif->comms->s, &bp, 0, (char *)&from, &fromlen);
	if( cnt == -1 ){
	  if( errno != EINTR && errno != EWOULDBLOCK ){
	    log(-1,"vara_rx: recv_mbuf failed with errno %d", errno );
	    break;
	  } 
	  if( Vara_debug ){
	    log( s, "vara_rx: recv_mbuf returned %d", errno );
	  }
	}

	if( cnt == 0 ){
	  log( s, "vara_rx: Connection closed" );
	  break;
	}

	if( bp == NULL )
	  continue;

	if( Vara_debug )
	  log( s, "vara_rx: Received %d bytes", cnt );
	
#ifdef J2_SNMPD_VARS
	/* 27Sep2011, Maiko (VE4KLM), This really should be tracked !!! */
	iface->rawrecbytes += cnt;
	iface->rawrecvcnt++;
	iface->lastrecv = secclock();
#endif
	vif->rxbytes++;

	/* If we're not connected, we should just let the buffer flush. */
	if( !vif->carrier ){
	  if( Vara_debug ){
	    log(-1,"vara_rx: No carrier, dropping data");
	  }
	  free_mbuf(bp);
	  bp = NULLBUF;
	  continue;
	}

	vara_recvdata( vif, bp );
	bp = NULLBUF;
      }

      log (s, "vara data rx disconnected");
      close_s(s);
    }

    return;
}

int32
vara_ioctl(ifp, cmd, set, val)
struct iface *ifp;
int cmd;
int set;
int32 val;
{
  /* not written */

  return -1;
}

void
vara_listen(on)
int on;
{
  char buf[11];
  struct ifvara *vif;
  int dev;

  if( Vara_debug )
    log (-1, "Calling vara_listen" );
  
  /* send listen command to all VARA interfaces */

  if( on ){
    strcpy( buf, "LISTEN ON" );
    Vara_listening = 1;
  } else {
    strcpy( buf, "LISTEN OFF" );
    Vara_listening = 0;
  }
  
  for( dev=0; dev < VARA_MAX; dev++ ){
    vif = &Vara[dev];
    if( vif->iface == NULL )
      continue;
    if( vif->ready == 0 )
      continue;

    vara_queuecommand( vif, buf );
  }
  
  return;
}

int
vara_connect(vap)
struct vara_cb *vap;
{
  struct ifvara *vif;
  char buf[30];
  char tmp[10];
  int ret;

  if( Vara_debug )
  log (-1, "Calling vara_connect" );

  
  vif = &Vara[vap->iface->dev];

  /* 
   * VARA can only support one connection at a time.  Fail
   * if someone attempts a second.
   */
  if( vif->busy )
    return -1;
  
  /* initiate connect */

  strcpy( buf, "CONNECT " );
  strcat( buf, pax25(tmp,vap->local));
  strcat( buf, " " );
  strcat( buf, pax25(tmp,vap->remote));

  vif->direction = VARADIROUT;

  ret = vara_queuecommand(vif,buf);
  if( ret == -1 )
    return -1;

  if( Vara_connectiontimeout > 0 ){
    set_timer( &vif->t1, Vara_connectiontimeout * 1000 );
    start_timer(&vif->t1);
  }
  
  return 0;
}

void
vara_show(ifp)
struct iface *ifp;
{
  /* not written */

  return;
}

void
vara_recvnewconn( vap, hdr )
struct ifvara *vap;
struct varahdr *hdr;
{
  struct mbuf *bp;
  int len;

  hdr->flag = VARAFCON;
  len = sizeof( struct varahdr );

  bp = qdata( (char *)hdr, len );

  if( Vara_debug )
    log (-1, "Calling vara_recvnewconn (%d bytes)", bp->cnt );

  if ( net_route( vap->iface, CL_VARA, bp ) != 0 ) {
    free_p(bp);
    bp = NULL;
  }

  return;
}

void
vara_recvendconn( vap, reason )
struct ifvara *vap;
char reason;
{
  struct mbuf *bp;
  struct varahdr hdr;
  int len;
  
  hdr.flag = VARAFDIS;
  memcpy( hdr.srcaddr, vap->srcaddr, 7 );
  memcpy( hdr.dstaddr, vap->dstaddr, 7 );
  hdr.conndir = vap->direction;
  hdr.reason = reason;

  len = sizeof( struct varahdr );

  bp = qdata( (char *)&hdr, len );

  if( Vara_debug )
    log (-1, "Calling vara_recvendconn (%d bytes)", bp->cnt );

  if ( net_route( vap->iface, CL_VARA, bp ) != 0 ) {
    free_p(bp);
    bp = NULL;
  }

  return;
}

void
vara_recvdata( vif, bp )
struct ifvara *vif;
struct mbuf *bp;
{
  struct varahdr hdr;
  int len;

  len = sizeof( struct varahdr );

  hdr.flag = VARAFDATA;
  memcpy( hdr.srcaddr, vif->srcaddr, 7 );
  memcpy( hdr.dstaddr, vif->dstaddr, 7 );
  hdr.conndir = vif->direction;

  bp = pushdown( bp, len );
  memcpy( bp->data, (char *)&hdr, len );
  
  if( Vara_debug )
    log( -1, "vara_recvdata: Sending up mbuf with %d bytes", bp->cnt );

  /* If it's PPP encapsulation, take a different path. */
  if( vif->iface->type == CL_PPP ){
    enqueue( &vif->rcvq, bp );

    return;
  } 

  /* Otherwise, take the VARA path */
  if ( net_route( vif->iface, CL_VARA, bp ) != 0 ) {
    free_p(bp);
    bp = NULL;
  }
  
  return;
}

void
vara_beacon( vif )
struct ifvara *vif;
{
  /* not written  */

  return;
}

void
vara_reset( vif )
struct ifvara *vif;
{
  /* not written  */

  return;
}

int
vara_output(iface,dest,source,type,data)
struct iface *iface;
char dest[];
char source[];
int16 type;
struct mbuf *data;
{
  /* not written */
  
  return -1;
}

int
vara_send(data,iface,gateway,prec,del,tput,rel)
struct mbuf *data;
struct iface *iface;
int32 gateway;
int prec;
int del;
int tput;
int rel;
{
  /* not writteN */
  
  return -1;
}

#endif

