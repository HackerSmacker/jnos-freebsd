#include "global.h"
#ifdef EA5HVK_VARA
/*
 *  start vara
 *  stop vara
 * 
 *  attach vara <interface> <mtu> <hostname> <port>
 *  vara ptt <interface> <type> [<hostname> <port>]
 *  vara reset <interface> 
 *  vara pppmode <interface> [enable|disable]
#ifdef _4Z1AC_VARAC
 *  vara kissport <interface> <hostname> <port>
 *  vara broadcast <interface> [on|off]
#endif
 *
 *  vara cmddebug [on|off]
 *  vara debug [on|off]
 *  vara timeout connection <seconds>
 *  vara timeout startup <seconds> <bytes>
 *  vara timeout transmit <seconds>
 *  vara timeout session <seconds>
 *
 */

#include "vara.h"
#include "cmdparse.h"
#include "pktdrvr.h"
#include "ax25.h"
#include "ppp.h"

extern int isboolcmd __ARGS((int *,char *));
extern int Vara_connectiontimeout;

/*
  vara listen [on|off]
  
  how to add ppp link
*/

char *Vara_ptttypes[] = {
  "VOX",
  "PERL extension",
  "FLRig"
};

/* mbox subcommand table */
static struct cmds DFAR Vatab[] = {
    { "command",      dovacommand,    0, 3, "vara command <interface> \"COMMAND\"" },
    { "cmddebug",     dovacmddebug,   0, 1, "vara cmddebug [on|off]" },
    { "debug",        dovadebug,      0, 1, "vara debug [on|off]" },
#ifdef _4Z1AC_VARAC
    { "broadcast",    dovabroadcast,  0, 2, "vara broadcast <interface> [on|off]" },
    { "kissport",     dovakissport,   0, 2, "vara kissport <interface> <hostname> <port>" },
#endif
    { "pppmode",      dovapppmode,    0, 2, "vara pppmode <interface> [on|off]" },
    { "ptt",          dovaptt,        0, 2, "vara ptt <interface> <type> [<hostname> <port>]" },
    { "reset",        dovareset,      0, 2, "vara reset <interface>" },
    { "timeout",      dovatimeout,    0, 2, "vara timeout [connection|startup|transmit|session] ..." },
    { NULLCHAR, NULL, 0, 0, NULLCHAR }
};

static struct cmds DFAR Vatotab[] = {
    { "connection",      dovatoconnection,    0, 1, NULLCHAR },
    { "startup",      dovatostartup,    0, 1, NULLCHAR },
    { "transmit",      dovatotransmit,    0, 1, NULLCHAR },
    { "session",      dovatosession,    0, 1, NULLCHAR },
    { NULLCHAR, NULL, 0, 0, NULLCHAR }
};

char *Varastates[] = {
    "",
    "Disconnected",
    "Listening",
    "Conn pending",
    "Disc pending",
    "Connected"
};

int
dovara(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    if(argc == 1)
        return dovastatus(0,NULL,NULL);
    return subcmd(Vatab,argc,argv,p);
}

#ifdef _4Z1AC_VARAC
int
dovakissport(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  char *ifacename = argv[1];
  char *hostname = argv[2];
  int port = atoi(argv[3]);
  struct iface *iface;
  struct ifvara *vif;
  
  iface = vara_getifacebyname( ifacename );
  if( iface == NULLIF ){
    log( -1, "vara_dovakissport: No such iface %s", ifacename );
    return 1;
  }

  vif = iface->vara;
  free( vif->kiss->hostname );
  vif->kiss->hostname = j2strdup( hostname );
  vif->kiss->port = port;

  /* signal kiss process */
  
  return 0;
}
#endif

int
dovaptt(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  char *ifacename = argv[1];
  char *type;
  struct iface *iface;
  struct ifvara *vif;

  iface = vara_getifacebyname( ifacename );
  if( iface == NULLIF ){
    tprintf( "No such interface: %s\n", ifacename );
    return 1;
  }

  vif = iface->vara;
  if( vif == NULL ){
    tprintf( "%s not a VARA interface\n", ifacename );
    return 1;
  }

  do {
    if( argc == 2 )
      break;
  
    type = argv[2];

    /* if PTT is already set, we need to unkey before we switch types */
    if( vif->ptttype && vif->pttonair ){
      varaptt( vif, "OFF" );
    }

    /* Reset */
    free( vif->ptt->hostname );
    vif->ptt->hostname = NULLCHAR;

    /* VOX is easy */
    if( !strcasecmp( type, "VOX" )){
      vif->ptttype = VARA_PTTVOX;
      break;
    }

    /* get the type for non-VOX. */
#ifdef FLRIG_PTT
    if( !strcasecmp( type, "FLRIG" )){
      vif->ptttype = VARA_PTTFLRIG;
    } else
#endif
	/*
	 * 29Aug2023, Maiko, Got rid of REMOTE_RTS_PTT definition,
	 * there's no point, and added new serial port control. I
	 * am finding the PERL solution is too slow for the PTT.
	 */
    if( !strcasecmp( type, "PERL" ))
      vif->ptttype = VARA_PTTPERL;
    else if( !strcasecmp( type, "SERIAL" ))
      vif->ptttype = VARA_PTTSERIAL;
    else
	{
      tprintf( "Unknown PTT type %s\n", type );
      return 1;
    }

	/* 29Aug2023, Maiko, Moving arg processing after setting the type,
	 * since the number of args is now dependent on ptt type ...
	 */
	if (vif->ptttype == VARA_PTTSERIAL && argc != 4)
	{
      j2tputs("vara ptt <ptttype> <comm port>\n");
      return 1;
	}
    /* The others require another connection. */
    else if( argc != 5 ){
      j2tputs("vara ptt <ptttype> <hostname> <port>\n");
      return 1;
    }

    free( vif->ptt->hostname );
    vif->ptt->hostname = j2strdup( argv[3] );
    vif->ptt->port = atoi( argv[4] );

  } while(0);
  
  tprintf( "%s PTT mode %s", ifacename,
	   Vara_ptttypes[vif->ptttype] );
  if( vif->ptttype == VARA_PTTVOX )
    tprintf( "to %s:%d", vif->ptt->hostname, vif->ptt->port );
  /* 29Aug2023, Maiko, passing comm port in hostname */
  else if( vif->ptttype == VARA_PTTSERIAL )
    tprintf( "to %s", vif->ptt->hostname);
  tprintf( "\n" );
  
  return 0;
}

int
dovareset(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  struct iface *iface;
  struct ifvara *vif;
  char *ifacename = argv[1];

  iface = vara_getifacebyname( ifacename );
  if( iface == NULLIF ){
    tprintf( "No such interface: %s\n", ifacename );
    return 1;
  }

  vif = iface->vara;
  if( vif == NULL ){
    tprintf( "%s not a VARA interface\n", ifacename );
    return 1;
  }

  /* Let supervisor process do the actual work. */
  vara_queuecommand( vif, "RESETINTERFACE" );

  tprintf( "Resetting %s\n", ifacename );
  
  return 0;
}

int
dovacommand(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  struct iface *iface;
  struct ifvara *vif;
  char *ifacename = argv[1];
  int ret;
  
  iface = vara_getifacebyname( ifacename );
  if( iface == NULLIF ){
    tprintf( "No such interface: %s\n", ifacename );
    return 1;
  }

  vif = iface->vara;
  if( vif == NULL ){
    tprintf( "%s not a VARA interface\n", ifacename );
    return 1;
  }

  /* Send the command */
  ret = vara_queuecommand( vif, argv[2] );
  if( ret == -1 ){
    tprintf( "interface %s not ready\n", ifacename );
    return -1;
  }

  return 0;
}

int
dovacmddebug(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  int ret;

  if( argc == 1 ){
    tprintf( "VARA command debugging is %s\n",Vara_cmddebug?"on":"off" );
    return 0;
  }
  
  ret = setbool(&Vara_cmddebug,"set VARA command debugging flag",argc,argv);

  tprintf( "VARA command debugging %s\n",Vara_cmddebug?"on":"off" );
  
  return ret;
}

int
dovadebug(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  int ret;
  
  if( argc == 1 ){
    tprintf( "VARA debugging is %s\n",Vara_debug?"on":"off");
    return 0;
  }

  ret = setbool(&Vara_debug,"set VARA debugging flag",argc,argv);

  tprintf( "VARA debugging %s\n",Vara_debug?"on":"off" );

  return ret;
}

#ifdef _4Z1AC_VARAC
int
dovabroadcast(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    return setbool(&Vara_broadcast,"set VARA broadcast",argc,argv);
}
#endif

int
dovapppmode(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  struct iface *iface;
  struct ifvara *vif;
  char *ifacename = argv[1];
  int val, ret;
  
  iface = vara_getifacebyname( ifacename );
  if( iface == NULLIF ){
    tprintf( "No such interface: %s\n", ifacename );
    return 1;
  }

  vif = iface->vara;
  if( vif == NULL ){
    tprintf( "%s not a VARA interface\n", ifacename );
    return 1;
  }

  if( argc == 2 ){
    tprintf( "%s pppmode %s\n", ifacename, (iface->type==CL_PPP)?"on":"off" );
    return 0;
  }

  ret = isboolcmd( &val, argv[2] );
  if( !ret ){
    tprintf( "Usage: vara pppmode <interface> <on|off>\n" );
    return 1;
  }

  if( val ){
    /* 
     * 01Sep2021, Maiko (VE4KLM), no more hard coding, one can now configure
     * the VARA interface as an IP Bridge (PPP), or to simply use for direct
     * access for forwarding of messages with other VARA based BBS systems.
     */

    setencap( iface, "PPP" );
    tprintf( "%s pppmode enabled\n", ifacename );
    log (vif->comms->s, "initializing ppp interface");

    if (ppp_init (iface))
      {
	log (-1, "ppp init failed");
	return -1;
      }
    
  } else {
    if( iface->type == CL_PPP ){
      log( vif->comms->s, "freeing ppp interface" );
      ppp_free( iface );
    }
    
    setencap( iface, "VARA" );
    tprintf( "%s pppmode disabled\n", ifacename );
  }
  
  return 0;
}

int
dovatimeout(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    return subcmd(Vatotab,argc,argv,p);
}

int
dovatostartup(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  switch( argc ){
  case 3:
    Vara_startuptimeout = atoi( argv[1] );
    if( Vara_startuptimeout < 0 )
      Vara_startuptimeout = 0;
    
    Vara_startupbytes = atoi( argv[2] );
    if( Vara_startupbytes < 0 )
      Vara_startupbytes = 0;
  case 1:
    tprintf( "VARA startup timeout: %d bytes in %d seconds\n",
	     Vara_startupbytes, Vara_startuptimeout );
    break;
  default:
    return 1;
  }

  return 0;
}

int
dovatosession(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  switch( argc ){
  case 2:
    Vara_sessiontimeout = atoi( argv[1] );
    if( Vara_sessiontimeout < 0 )
      Vara_sessiontimeout = 0;
  case 1:
    tprintf( "VARA session timeout: %d seconds\n",
	     Vara_sessiontimeout );
    break;
  default:
    return 1;
  }

  return 0;
}

int
dovatotransmit(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  switch( argc ){
  case 2:
    Vara_transmittimeout = atoi( argv[1] );
    if( Vara_transmittimeout < 0 )
      Vara_transmittimeout = 0;
  case 1:
    tprintf( "VARA transmit timeout: %d seconds\n",
	     Vara_transmittimeout );
    break;
  default:
    return 1;
  }

  return 0;
}

int
dovatoconnection(argc,argv,p)
int argc;
char *argv[];
void *p;
{
  switch( argc ){
  case 2:
    Vara_connectiontimeout = atoi( argv[1] );
    if( Vara_connectiontimeout < 0 )
      Vara_connectiontimeout = 0;
  case 1:
    tprintf( "VARA connection timeout: %d seconds\n",
	     Vara_connectiontimeout );
    break;
  default:
    return 1;
  }

  return 0;
}

int
dovastatus(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    register struct vara_cb *vap;
    char tmp[AXBUF];
    char tmp2[AXBUF];
    int dev;
    struct ifvara *vif;
  
    if(argc < 2){
#ifdef UNIX
        /* 09Mar2021, Maiko, AXB is now up to 12 in width, give Iface 5 more spaces */
        tprintf("&AXB         Snd-Q   Rcv-Q   Remote    Local     Iface       State\n");
#else
        j2tputs("&AXB Snd-Q   Rcv-Q   Remote    Local     Iface  State\n");
#endif
        for(vap = Vara_cb;vap != NULLVARA; vap = vap->next){
#ifdef UNIX
            /* 09Mar2021, Maiko, AXB is now up to 12 in width */
            if(tprintf("%-12lx %5d   %5d   %-10s%-10s%-12s%s\n",
#else
                if(tprintf("%4.4x %-8d%-8d%-10s%-10s%-7s%s\n",
#endif
                    FP_SEG(vap),
                    len_q(vap->txq),len_p(vap->rxq),
                    pax25(tmp,vap->remote),
                    pax25(tmp2,vap->local),
                    vap->iface?vap->iface->name:"",
		    Varastates[vap->state]) == EOF)
                    return 0;
        }

       j2tputs("\n");
       for( dev = 0; dev < VARA_MAX; dev++ ){
	 vif = &Vara[dev];
	 if( vif->iface == NULLIF )
	   continue;
	 
	 tprintf( "iface %s (%s)\n", vif->iface->name, vif->iface->iftype->name );
	 
	 j2tputs("  Heartbeat: ");
	 if(run_timer(&vif->heartbeat))
	   tprintf("%d",read_timer(&vif->heartbeat));
	 else
	   j2tputs("stop");
	 tprintf("/%d ms\n",dur_timer(&vif->heartbeat));

	 j2tputs("  TXwatchdog: ");
	 if(run_timer(&vif->txwatchdog))
	   tprintf("%d",read_timer(&vif->txwatchdog));
	 else
	   j2tputs("stop");
	 tprintf("/%d ms\n",dur_timer(&vif->txwatchdog));

       	 j2tputs("  Connection: ");
	 if(run_timer(&vif->t1))
	   tprintf("%d",read_timer(&vif->t1));
	 else
	   j2tputs("stop");
	 tprintf("/%d ms\n",dur_timer(&vif->t1));

	 j2tputs("  Progression: ");
	 if(run_timer(&vif->t2))
	   tprintf("%d",read_timer(&vif->t2));
	 else
	   j2tputs("stop");
	 tprintf("/%d ms\n",dur_timer(&vif->t2));

	 j2tputs("  Session: ");
	 if(run_timer(&vif->t3))
	   tprintf("%d",read_timer(&vif->t3));
	 else
	   j2tputs("stop");
	 tprintf("/%d ms\n",dur_timer(&vif->t3));

	 tprintf( "\n" );
       }
	       
        return 0;
    }

	/* 31May2007, Maiko (VE4KLM), new getax25cb() function */
	if ((vap = getvaracb (argv[1])) == NULLVARA)
		j2tputs (Notval);
	else
		st_vara (vap);

    return 0;

}

/* Dump one control block */
void
st_vara(vap)
register struct vara_cb *vap;
{
  char tmp[AXBUF];
  char tmp2[AXBUF];

  if(vap == NULLVARA)
    return;

#ifdef UNIX
    /* 09Mar2021, Maiko, AXB is now up to 12 in width */
    tprintf("&AXB         Local     Remote    Iface  State\n");
    tprintf("%-12lx %-9s %-9s %-6s",
	    FP_SEG(vap),
#else
    j2tputs("&AXB Local     Remote    Iface  State\n");
    tprintf("%4.4x %-9s %-9s %-6s",
	    FP_SEG(vap),
#endif
    pax25(tmp,vap->local),
    pax25(tmp2,vap->remote),
    vap->iface?vap->iface->name:"" );
	    
    tprintf(" %s\n",Varastates[vap->state]);

    return;
}

#endif
