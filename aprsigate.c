/* 
 * APRS I-gate
 */

#include "global.h"

#ifdef APRSD

/* Connection info for each igate connection. */
struct igateconn {
  char *hostname;
  int port;
  int s;
};

struct ifigate {
};


int igate_attach (int argc, char *argv[], void *p)
{
  struct iface *ifp;
  int dev, len;

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

  return 0;
}


int
igate_stop(iface)
struct iface *iface;
{
  struct ifvara *vif;

  vif = &Vara[iface->dev];
  iface->vara = NULL;
  
  return 0;
}

igate_raw()
{
}

igate_rx()
{
}

igate_tx()
{
}


#endif
