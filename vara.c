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
#include "mbuf.h"
#include "mailbox.h"
#include "iface.h"
#include "trace.h"
#include "pktdrvr.h"
#include "devparam.h"
#include "udp.h"
#include "vara.h"
#include "socket.h"

extern int getusage (char*, char*);
extern char Myalias[AXALEN];
extern char Ccall[AXALEN];
extern char Ttycall[AXALEN];
extern char tncall[AXALEN];

/*
 * Maiko, new function in main.c, modified 24Jan2021 so I can
 * output direct to the JNOS log instead of the usual console,
 * so now has an extra argument, if -1L stick with console.
 */
extern int j2directsource (int, char*);

/*
 * 29Sep2019, Maiko (VE4KLM), struct breakouts
 */
#include "ax25.h"
#include "netuser.h"

#include <ctype.h>	/* for isprint() */

#include "usock.h"	/* 20Apr2021, maiko */

#define UNUSED(x) ((void)(x))   /* 15Apr2016, VE4KLM, trick to suppress any
                                 * unused variable warning, why are there so
                                 * many MACROs in this code, should convert
                                 * them to actual functions at some point. */
typedef struct varaparm {
	char *hostname;
	int port;
} VARAPARAM;


#if 0
#ifdef  JNOS_DEFAULT_NOIAC
/* #warning "default NO IAC processing in affect" */
        struct usock *up;
#endif
  
/*
 * 31Aug2021, Maiko (VE4KLM), This seems very important when talking
 * with the VARA modem over tcp/ip, I keep forgetting about the NOIAC
 * option that I put in 5 or so years ago, hopefully this is it !!!
 */
#ifdef  JNOS_DEFAULT_NOIAC
	if (!vara_ip_bridge_mode)
	{
    if ((up = itop (s)) == NULLUSOCK)
      log (-1, "telnet (itop failed) unable to set options");
	else
		up->flag |= SOCK_NO_IAC;
	}
#endif
#endif


void
vara_recv(iface,bp)
struct iface *iface;
struct mbuf *bp;
{
  struct varahdr hdr;
  struct vara_cb *vap = NULLVARA;
  struct ifvara *vif;
  int len, cnt;
  int To_us;
  char tmp[10];
  
  len = sizeof( struct varahdr );
  cnt = pullup( &bp, (char *)&hdr, len );

  if( Vara_debug && bp )
    log( -1, "vara_recv: Received mbuf with %d bytes", cnt );
  
  if( hdr.conndir != VARA_INBOUND && hdr.conndir != VARA_OUTBOUND ){
    log( -1, "vara_recv: unknown conndir %d", hdr.conndir );
    free_mbuf( bp );
    return;
  }

  switch( hdr.flag ){
  case VARAFCON:
    /* add route, if there were digipeaters */
    /* TODO */

    if( hdr.conndir == VARA_OUTBOUND ){
      /* find waiting sock */
      vap = find_vara( NULL,hdr.dstaddr,iface,hdr.conndir);
      if( vap == NULLVARA ){
	/* Our requestor disappeared. */
	log( -1, "vara_recv: connection succeeded for unknown socket" );
	reset_varabyiface(iface);

	free_mbuf( bp );
	return;
      }

      vap->state = VARASTCON;
      memcpy(vap->local,hdr.srcaddr,AXALEN);
      memcpy(vap->remote,hdr.dstaddr,AXALEN);

      break;
	}
  
    if( hdr.conndir != VARA_INBOUND ){
      log( -1, "vara_recv: unknown conndir %d", hdr.conndir );
      break;
    }

    /* verify Vai_sock is listening */
    if( !Vara_listening ){
      log( -1, "vara_recv: received connection even though we weren't listening" );
      free_mbuf( bp );

      /* Make sure all interfaces are not listening */
	vara_listen( 0 );

	return;
    }

    /* create new socket */
    vif = &Vara[iface->dev];

  /* NOTE - 29Aug2023, Maiko, Why is 'vif' set but not used ??? */

    vap = cr_vara(hdr.dstaddr,hdr.srcaddr,iface,hdr.conndir);
    vap->state = VARASTCON;


    /* figure out which link it is */
    if( Vara_debug )
      log(-1,"vara_recv: call came in to %s", pax25(tmp,(char *)&hdr.dstaddr));

    if(!memcmp((char *)&hdr.dstaddr,Myalias,ALEN))
      To_us = ALIAS_LINK;     /* this is for the alias */
#ifdef NETROM
    else if((iface->flags & IS_NR_IFACE) && addreq((char *)&hdr.dstaddr,Nr_iface->hwaddr))
      To_us = NETROM_LINK;    /* this is for the netrom callsign! */
#endif
    else if(addreq((char *)&hdr.dstaddr,iface->hwaddr))
      To_us = IFACE_LINK;     /* this is to the interface call */
#ifdef MAILBOX
    else if(addreq((char *)&hdr.dstaddr,Bbscall))
      To_us = ALIAS_LINK;       /* This will jumpstart */
#endif
#ifdef CONVERS
    else if((iface->flags & IS_CONV_IFACE) && addreq((char *)&hdr.dstaddr,Ccall))
      To_us = CONF_LINK;      /* this is for the conference call */
#endif
#ifdef TTYCALL
    else if (addreq((char *)&hdr.dstaddr, Ttycall))
      To_us = TTY_LINK;
#endif
#ifdef TNCALL
    else if (addreq((char *)&hdr.dstaddr, tncall))
      To_us = TN_LINK;
#endif

    /* set up jumpstart if needed */
    vap->jumpstarted = To_us;

    if(vap->jumpstarted & (ALIAS_LINK+CONF_LINK+TTY_LINK+TN_LINK)){
      if( Vara_debug )
	log(-1,"vara_recv: setting jumpstart" );
      vap->jumpstarted += JUMPSTARTED;
				}

    /* if any other sock is open, close it, because VARA
     * can only have one connection. */
    vara_cb_closeallbut( iface, vap, EBUSY );

    free_mbuf( bp );

    if( Vara_debug )
      log( -1, "vara_recv: Got new connection, calling r_upcall" );

    if(vap->r_upcall != NULLVFP((struct vara_cb*,int)))
      (*vap->r_upcall)(vap,len_p(vap->rxq));

    break;
  case VARAFDIS:
    /* close any sock that is open */
    vara_cb_closeallbut( iface, NULL, hdr.reason );

    free_mbuf( bp );

    break;
  case VARAFDATA:
    if( Vara_debug )
      log(-1,"vara_recv: Sending data mbuf up with %d bytes", bp->cnt );

    /* find the cb */
    if( hdr.conndir == VARA_OUTBOUND ){
      vap = find_vara(hdr.srcaddr,hdr.dstaddr,iface,VARADIROUT);
    } else {
      vap = find_vara(hdr.dstaddr,hdr.srcaddr,iface,VARADIRIN);
	}

    if( vap == NULLVARA ){
      if( Vara_debug )
	log(-1,"vara_recv: vap == NULLBUF");

      free_mbuf( bp );
      return;
	}

    /* put the data on rxq */
    append(&vap->rxq,bp);
    if(vap->r_upcall != NULLVFP((struct vara_cb*,int)))
      (*vap->r_upcall)(vap,len_p(vap->rxq));

					break;
				}

 return;
}


#endif	/* End of EA5HVK_VARA */
