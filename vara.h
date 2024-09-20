 /* Mods by WZ0C */
#ifndef _VARA_H
#define _VARA_H
  
#ifndef _GLOBAL_H
#include "global.h"
#endif
  
#ifndef _IP_H
#include "ip.h"
#endif
  
#ifndef _MBUF_H
#include "mbuf.h"
#endif
  
#ifndef _IFACE_H
#include "iface.h"
#endif
  
#ifndef _SOCKADDR_H
#include "sockaddr.h"
#endif

extern int Vara_listening;
extern int Vara_startuptimeout;
extern int Vara_startupbytes;
extern int Vara_transmittimeout;
extern int Vara_sessiontimeout;
extern int Vara_broadcast;
extern int Vara_debug;
extern int Vara_cmddebug;

#define VARA_EOL      "\r"    /* VARA end-of-line convention */

/* VARA sub-layer definitions */

#define VARAMAXDIGIS    3   /* Maximum number of digipeaters */
#define AXALEN      7   /* Total AX.25 address length, including SSID */

#define VARA_MAX    4   /* Assuming no more than 4 VARA modems */

/* Codes for the open_vara call */
#define VA_PASSIVE  0
#define VA_ACTIVE   1
#define VA_SERVER   2   /* Passive, clone on opening */

/* Direction of a VARA connection */
#define VARA_OUTBOUND 1
#define VARA_INBOUND  2

#define ILEN 6

#define VARA_TEXT   1
#ifdef _4Z1AC_VARAC
#define VARA_KISS   2
#endif


/* The calls we've configured VARA to use. */
extern char Varacalls[5][10];

/* Connection info for each connection VARA needs to the modem. */
struct varaconn {
  char *hostname;
  int port;
  int s;
};

struct varacmd {
  char *command;
  struct varacmd *next;
};

struct sockaddr_va {
    short sva_family;
    char va_addr[7];
    char iface_index;
    char filler[ILEN];
};

struct varahdr {
  char flag;
  char srcaddr[7];
  char dstaddr[7];
  char digi1[7];
  char digi2[7];
  char digi3[7];
  int conndir;
  int reason;

#define VARAFCON   1
#define VARAFDIS   2
#define VARAFDATA  3
};

struct vara_cb {
  struct vara_cb *next;

  struct iface *iface;
  int conndir;

  struct mbuf *txq;       /* Transmit queue */
  struct mbuf *rxq;       /* Receive queue */
  
  char local[AXALEN];     /* Addresses */
  char remote[AXALEN];

  struct {
    char clone;         /* Server-type cb, will be cloned */
  } flags;

  int paclen;
  int window;
  
  char reason;            /* Reason for connection closing */
#define VA_NORMAL   0     /* Normal close */
#define VA_DM       1     /* Received DM from other end */
#define VA_TIMEOUT  2     /* Excessive retries */
#define VA_UNUSED   3     /* Link is redundant - unused */
#define VA_IOERROR  4     /* Lost connection to the modem */
  
    int state;          /* Link state */
#define VARASTDISC   1           /* disconnected */
#define VARASTLISTEN 2           /* listening for incoming connections */
#define VARASTCPEND  3           /* connection pending */
#define VARASTDPEND  4           /* disconnect requested locally */
#define VARASTCON    5           /* connected */

    void (*r_upcall) __ARGS((struct vara_cb *,int));    /* Receiver upcall */
    void (*t_upcall) __ARGS((struct vara_cb *,int));    /* Transmit upcall */
    void (*s_upcall) __ARGS((struct vara_cb *,int,int));    /* State change upcall */

    int user;           /* User pointer */
  
    int jumpstarted;    /* Was this one jumpstarted ? */
};

/* VARA data kept per interface */
struct ifvara {
  struct iface *iface;

  int ready;

#ifdef _4Z1AC_VARAC
  struct proc *kissproc;    /* KISS receiver process, if any */
#endif

  /****************************************
   * Modem connection                     *
   ***************************************/
  
  /* The TCP connections to the VARA application. */
  struct varaconn *command;
  struct varaconn *comms;
#ifdef _4Z1AC_VARAC
  struct varaconn *kiss;
#endif

  /* Make sure the control port is still alive. */
  struct timer heartbeat;

  /* Make sure we don't transmit too long. */
  struct timer txwatchdog;

  /* Ensure the connection happens. */
  struct timer t1;

  /* Ensure connection ramps up. */
  struct timer t2;

  /* Session timeout */
  struct timer t3;
  
  /* VARA doesn't have rig control.  Need to be able 
   * to control the PTT another way. */
#define VARA_PTTVOX   0
#define VARA_PTTPERL  1
#define VARA_PTTFLRIG 2
#define VARA_PTTSERIAL 3	/* 29Aug2023, Maiko, Direct comm port ctrl */
  struct varaconn *ptt;
  int ptttype;
  int pttonair;
  
  /* Queue of commands to send to the command channel. */
  struct varacmd *cmdq;

  int didreset;       /* Did we just reset the interface? */

  /****************************************
   * Connection parameters                *
   ***************************************/
  
  /* bandwidth for outgoing connections */
  char *outbw;

  /* bandwidth for incoming connections */
  char *inbw;

  /* txq */
  int txq;
  int txints;
  int txget;
  int txblock;
  int txovq;
  
  /****************************************
   * Connection state                     *
   ***************************************/

  /* Listen enable.  Do we accept inbound connections? */
  int listen; 

  /* is channel busy? */
  /*                 busy:
		     The modem is engaged with another station.
                     Outgoing from CONNECT to DISCONNECTED.
                     Incoming from CONNPENDING to DISCONNECTED.
		     Incoming from CONNPENDING to CANCELPENDING.
  */
  int busy;         /* The modem has an attempted or active connection. */
  int txactive;     /* The modem has data queued to transmit. */

  /* do we have an active connection? */
  int carrier; 

  /* stations */
  char srcaddr[7];
  char dstaddr[7];
  
  /* is connection outbound or inbound */
  int direction;
#define VARADIROUT  1
#define VARADIRIN   2

  /* Time the current connection started. */
  time_t starttime;

  /* Bandwidth of current connection */
  char *bandwidth; 

  /****************************************
   * Data for this connection             *
   ***************************************/

  /* number of bytes given to modem to transmit */
  int txbytes;    

  /* number of bytes in modem buffer */
  int txbuf;      

  /* number of bytes received by modem for us */
  int rxbytes;

  /* data to be sent */
  struct mbuf *sndq;

  /* data received for other encapsulation */
  struct mbuf *rcvq;

  /* busy time for this interface */
  int attachtime;   /* the start of the measurement timeframe */
  int busystart;    /* when the current busy period started */
  int busytime;     /* total busytime since the interface attach */
 
};

/* vara.c */
void vara_recv __ARGS((struct iface *,struct mbuf *));
int disc_vara __ARGS((struct vara_cb *));

/* varauser.c */
int reset_vara __ARGS((struct vara_cb *));
int reset_varabyiface __ARGS((struct iface *));
int get_vara __ARGS((int));
int vara_sendbuf __ARGS((int, struct mbuf *));
struct vara_cb *getvaracb __ARGS((char *));
struct vara_cb *open_vara __ARGS((struct iface *,char *,char *,int,int16,
  void (*) __ARGS((struct vara_cb *,int)),
  void (*) __ARGS((struct vara_cb *,int)),
  void (*) __ARGS((struct vara_cb *,int,int)),
  int));
int kick_vara(struct vara_cb *);
struct mbuf *recv_vara(struct vara_cb *,int16);
int send_vara(struct vara_cb *,struct mbuf *);
int va_output(struct iface *,char *,char *,int16,struct mbuf *data);

/* varasubr.c */
struct vara_cb *cr_vara __ARGS((char *,char *,struct iface *,int));
void del_vara __ARGS((struct vara_cb *axp));
struct vara_cb *find_vara __ARGS((char *local,char *remote,struct iface *iface,int));
int vara_ismycall __ARGS((char *call));
void vara_cb_closeallbut __ARGS((struct iface *,struct vara_cb *,int));
char *vara_getcalls();
struct iface *vara_getifacebyname(char *);

/* varamodem.c */
void vara_listen __ARGS((int));
void vara_supv __ARGS((int,void *,void *));
void vara_tx __ARGS((int,void *,void *));
void vara_rx __ARGS((int,void *,void *));
int vara_ipconnect __ARGS((char *,int,int));
int vara_connect __ARGS((struct vara_cb *));
int vara_output __ARGS((struct iface *, char [], char [],int16,struct mbuf *));
int vara_send __ARGS((struct mbuf *,struct iface *,int32,int,int,int,int));
void vara_recvdata __ARGS((struct ifvara *,struct mbuf *));
void vara_flushcommandqueue __ARGS((struct ifvara *vif));
void vara_recvendconn __ARGS((struct ifvara *,char));
int vara_send __ARGS((struct mbuf *bp,struct iface *iface,int32 gateway,int prec,
    int del,int tput,int rel));
int vara_output __ARGS((struct iface *iface,char *dest,char *source,int16 pid,
    struct mbuf *data));
int32 vara_ioctl(struct iface *,int,int,int32);
void vara_show(struct iface *);

/* varacli.c */
int vara_stop __ARGS((struct iface *));
int vara_initinterface __ARGS((int,struct iface *,struct ifvara *));
int vara_initmodem __ARGS((struct ifvara *));
int vara_queuecommand __ARGS((struct ifvara *,char *));
char *vara_dequeuecommand __ARGS(( struct ifvara * ));
void vara_sessiontimedout __ARGS((void *));
void vara_connecttimedout __ARGS((void *));
void vara_connectionrampup __ARGS((void *));

/* varaptt.c */
void varaptt __ARGS((struct ifvara *,char *));
void varapttperl __ARGS((struct ifvara *, char *));
void varapttflrig __ARGS(( struct ifvara *, int ));
void varapttserial __ARGS(( struct ifvara *, int ));	/* 29Aug2023, Maiko */

/* varacmd.c */
int dovaptt __ARGS((int,char **,void *));
int dovareset __ARGS((int,char **,void *));
int dovacommand __ARGS((int,char **,void *));
int dovacmddebug __ARGS((int,char **,void *));
int dovadebug __ARGS((int,char **,void *));
#ifdef _4Z1AC_VARAC
int dovakissport __ARGS((int,char **,void *));
int dovabroadcast __ARGS((int,char **,void *));
#endif
int dovapppmode __ARGS((int,char **,void *));
int dovatimeout __ARGS((int,char **,void *));
int dovatostartup __ARGS((int,char **,void *));
int dovatosession __ARGS((int,char **,void *));
int dovatotransmit __ARGS((int,char **,void *));
int dovastatus __ARGS((int,char **,void *));
int dovatoconnection __ARGS((int,char **,void *));
void st_vara __ARGS((register struct vara_cb *));

/* varamail.c */
int varastart __ARGS((int argc,char *argv[],void *p));
int vara0 __ARGS((int argc,char *argv[],void *p));

/* socket.c */
void s_vrcall(struct vara_cb *,int);
void s_vtcall(struct vara_cb *,int);
void s_vscall(register struct vara_cb *,int,int);


#define NULLVARA    ((struct vara_cb *)0)
#define NULLVAADDR  ((char *)0)

extern struct ifvara Vara[VARA_MAX];
extern struct vara_cb *Vara_cb;

#endif  /* _VARA_H */
