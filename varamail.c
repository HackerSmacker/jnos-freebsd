/* VARA mailbox interface
 *
 */
#include "global.h"
#ifdef EA5HVK_VARA
#include "socket.h"
#include "lapb.h"
#include "session.h"
#include "usock.h"
#include "vara.h"
  
/* Vai_sock is kept in Socket.c, so that this module won't be called */
extern void conv_incom(int,void *,void *);
  
int
varastart(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    int s,whatcall;
    struct usock *up;
    long whatcall_l;	/* 01Oct2009, Maiko, 64 bit warnings */ 

    if (Vai_sock != -1)
        return 0;
  
    j2psignal(Curproc,0); /* Don't keep the parser waiting */
    chname(Curproc,"VARA listener");
    Vai_sock = j2socket(AF_VARA,SOCK_STREAM,0);
    /* bind() is done automatically */
    if(j2listen(Vai_sock,1) == -1){
        close_s(Vai_sock);
        return -1;
    }
    for(;;){
        if((s = j2accept(Vai_sock,NULLCHAR,NULLINT)) == -1)
            break;  /* Service is shutting down */
        /* Spawn a server */

	if( Vara_debug )
	  log(-1,"varastart: accepted connection");
	
        up = itop(s);
        whatcall = up->cb.vara->jumpstarted;
        /* Check to see if jumpstart was actually used for
         * this connection.
         * If not, then again eat the line triggering this all
         */
        if(!(whatcall&JUMPSTARTED)) {
	  if( Vara_debug )
	    log(-1,"varastart: not jumpstarted");
	  sockmode(s,SOCK_ASCII);    /* To make recvline work */
	  recvline(s,NULLCHAR,80);
        }
        /* Now find the right process to start - WG7J */
#ifdef CONVERS
        if(whatcall & CONF_LINK) {
	  if( Vara_debug ){
	    log( -1, "Starting CONVERS Server" );
	  }
	  if(newproc("CONVERS Server",1048,conv_incom,s,(void *)VARAMODEM,NULL,0) == NULLPROC)
	    close_s(s);
	  continue;
        }
#endif
  
#ifdef MAILBOX
  
#ifdef TTYCALL
        if(whatcall & TTY_LINK) {
	  if( Vara_debug )
	    log( -1, "Starting TTYLINK Server" );
	  if(newproc("TTYLINK Server",2048,ttylhandle,s,(void *)VARAMODEM,NULL,0) == NULLPROC)
	    close_s(s);
	  continue;
        }
#endif
	
#ifdef TNCALL
        if(whatcall & TN_LINK) {
	  if( Vara_debug )
	    log( -1, "Starting tnlink" );
	  if(newproc("tnlink",2048,tnlhandle,s,(void *)VARAMODEM,NULL,0) == NULLPROC)
	    close_s(s);
	  continue;
        }
#endif
	whatcall_l = (long)whatcall;	/* 01Oct2009, Maiko, 64 bit warning */
	
	if( Vara_debug )
	  log( -1, "Trying to start MBOX Server" );
        if (newproc ("MBOX Server", 2048, mbx_incom, s, (void*)whatcall_l, NULL, 0) == NULLPROC)
	  close_s (s);
	
#else /* no MAILBOX ! */

	if( Vara_debug )
	  log( -1, "Starting TTYLINK Server" );
	
        /* Anything now goes to ttylink */
        if(newproc("TTYLINK Server",2048,ttylhandle,s,(void *)VARAMODEM,NULL,0) == NULLPROC)
            close_s(s);
  
#endif /* MAILBOX */
  
    }
    close_s(Vai_sock);
    Vai_sock = -1;
    return 0;
}
  
int
vara0(argc,argv,p)
int argc;
char *argv[];
void *p;
{
    close_s(Vai_sock);
    Vai_sock = -1;
    return 0;
}
  
#endif /* VARASERVER */
