#include "global.h"
#ifdef EA5HVK_VARA
#include "vara.h"

#include "netuser.h"	/* 06Jul2023, Maiko */
#include "udp.h"	/* 09Jul2023, Maiko, prototype send_udp() */

#include <fcntl.h>
#include <sys/ioctl.h>

/* VARA doesn't control the radio on its own. */

int trigger_RTS_via_perl_listener ( struct varaconn *, char * );

void
varaptt( vif, onoff )
struct ifvara *vif;
char *onoff;
{
  int mode = vif->ptttype; /* 08Jul2023, Maiko, was hardcode to 0 before */

  /* if onoff == "ON", set 3 minute timer */

  if( !strcmp( onoff, "ON" )){
    vif->pttonair = 1;

    set_timer( &vif->txwatchdog, Vara_transmittimeout * 1000 );
    start_timer( &vif->txwatchdog );
    
  } else if( !strcmp( onoff, "OFF" )){
    stop_timer( &vif->txwatchdog );
    vif->pttonair = 0;
  }
  
  if( mode == VARA_PTTVOX )
    return;
  else if( mode == VARA_PTTPERL )
    varapttperl( vif, vif->pttonair?"PTT ON":"PTT OFF" );
/*
 * 29Aug2023, Maiko, Direct control of serial port on the linux
 * box that JNOS runs on. Actually wrote this back in mid July,
 * when I connected with WZ0C on 17m. I'm not so sure the PERL
 * solution is a good idea anymore, latency is too high, and I
 * was finding the PTT too slow and VARA was not working.
 *  (and got rid of REMOTE_RTS_PTT definition, there's no point)
 *
 * The only issue now is my linux box has to be in the same
 * shack as the radio interface circuit (it usually is) ...
 */
  else if( mode == VARA_PTTSERIAL )
    varapttserial( vif, vif->pttonair);
  else if( mode == VARA_PTTFLRIG )
    varapttflrig( vif, vif->pttonair );

  return;
}

/*
 * 29Aug2023, Maiko (VE4KLM), direct control of linux serial port,
 * actually wrote this mid July already during tests on 17m band.
 */
void
varapttserial(vap,cmd)
struct ifvara *vap;
int cmd;
{
	static int ttyfd = 0;

	int RTS_flag = TIOCM_RTS;

	/* 29Aug2023, Maiko, no more hardcoding of comm port, pass in hostname
		ttyfd = open ("/dev/ttyS0", O_RDWR | O_NOCTTY);
	 */
	if (!ttyfd)
		ttyfd = open (vap->ptt->hostname, O_RDWR | O_NOCTTY);

	if (cmd)
		ioctl (ttyfd, TIOCMBIS, &RTS_flag);
	else
		ioctl (ttyfd, TIOCMBIC, &RTS_flag);
}

void
varapttperl(vap,cmd)
struct ifvara *vap;
char *cmd;
{
  if( vap->ptt == NULL )
    return;
  
  trigger_RTS_via_perl_listener( vap->ptt, cmd );

  return;
}

void
varapttflrig(vif,op)
struct ifvara *vif;
int op;
{
}

/*
 * 28Jan2021, Maiko (VE4KLM), function to send UDP frame to a perl
 * listener I wrote that runs on the windows machine running VARA
 * modem, and triggers the RTS pin on that windows machine serial
 * port for PTT without needing Signal Link or any DRA board.
 *
 * Snippets taken from the fldigi.c code I wrote in 2015 !
 *
 * 29Jan2021, Added ifpp, so ip address is simply taken from
 * the attach vara command, most likely (as in my case), any
 * serial port will be on the same PC as VARA modem uses.
 *  (had hardcoded it before, call it lazy development)
 */
int
trigger_RTS_via_perl_listener (conn,cmdstr)
struct varaconn *conn;
char *cmdstr;
{
  struct mbuf *bp;
  struct socket lsock, rsock;
  int len = strlen (cmdstr);
  
  lsock.address = INADDR_ANY;
  lsock.port = 8400;
  
  rsock.address = resolve (conn->hostname);
  rsock.port = conn->port;
  
  if ((bp = alloc_mbuf (len)) == NULLBUF)
    return 0;
  
  /* do NOT use string copy, string terminator will corrupt memory */
  memcpy ((char*)bp->data, cmdstr, len);
  
  bp->cnt = (int16)len;
  
  return send_udp (&lsock, &rsock, 0, 0, bp, 0, 0, 0);
}
#endif
