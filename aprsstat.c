
/*
 * APRS Services for JNOS 2.0, JNOS 111x, TNOS 2.30 & TNOS 2.4x
 *
 * April,2001-June,2004 - Release (C-)1.16+
 *
 * Designed and written by VE4KLM, Maiko Langelaar
 *
 * For non-commercial use (Ham Radio) only !!!
 *
 * 13Dec2023, Maiko (VE4KLM) finalized change of the original
 * heard list code to use a link list approach, a much better
 * approach, and in line with the 'standard' ax25 heard list.
 *
 * Also decided to get rid of the 'combined interface' approach to
 * how aprs status information is displayed, so now each interface
 * has it's own section, easier to pick out the callsigns, etc.
 * 
 * This particular source already includes the mods from the first
 * aprs.patch from 30Oct2023, by Michael (WZ0C), so any changes to
 * this source (aprsstat.c) have been removed from the patch file.
 *
 * I had originally migrated the 'last == 0' (as time == 0), but now
 * am seeing it was simply used to indicate empty entries in original
 * heard list array, to reuse entries ? and not have to grow the array,
 * so that portion of code is no more. With link lists, the entries are
 * deleted, not reused - hopefully I have interpreted this correctly.
 */

#include "global.h"

#ifdef	APRSD
#include <unistd.h>

#include "mbuf.h"

/* 01Feb2002, Maiko, DOS compile for JNOS will complain without these */
#ifdef	MSDOS
#include "iface.h"
#include "arp.h"
#include "slip.h"
#endif

#include "ax25.h"

#include <time.h>

#include "aprs.h"	/* 16May2001, VE4KLM, Finally added prototypes */

#include "pktdrvr.h"	/* 15Nov2023, Maiko */

#include "files.h"	/* WZ0C original aprs.patch mods */

static int maxaprshrd = 0;

/*
 * 10Oct2023, I don't like how the APRS status is displayed, I would like it
 * sorted by latest time heard, and separate listing for each APRS interface,
 * resorting to link lists - doing it the way regular ax25 heard gets done.
 */

struct aq {

	/* standard ax25 heard structure */

	struct aq *next;
	char addr[AXALEN];
	struct iface *iface;
	int32 time;
	int32 currxcnt;

	/* APRS specific information we want to track */

	char dti;
	int ndigis;
	char digis[MAXDIGIS][AXALEN];
};

#define	NULLAQ (struct aq*)0

struct aq *Aq;	/* APRS Heard link list */

int numaq;	/* 12Oct2023, Maiko, number of entries in the link list */

/*
 * 31May2001, VE4KLM, Heard list can now be dynamically resized by the
 * sysop while TNOS is running, and is now initialized in autoexec.nos
 * file. Previously I had a hardcoded array of 20 entries max.
 * 12Oct2023, Maiko (VE4KLM), Now using link lists, this function will
 * disappear anyways, create new aprs subcommand instead (this becomes
 * an empty function, just to resolve early development link errors)
 */
void init_aprshrd (int hsize)
{
	maxaprshrd = hsize;		/* should probably flush the list, temporary */
}


#ifdef	APRS_14501

/*
 * 03Aug2001, VE4KLM, To calculate the amount of memory to allocate
 * for the 14501 page, we need to know the current number of entries
 * in the APRS heard list.
 *
 * 12Oct2023, Maiko, Using link lists now like is done with regular ax25 heard
 */
int get_topcnt (void)
{
	return numaq;
}

#endif

/*
 * 17Oct2001, Maiko, New Function returns a pointer to a string
 * that identifies the name of the interface on which the passed
 * call was last heard on. The passed call is in internal format.
 */

const char *iface_hrd_on (char *callsign)
{
	register struct aq *ap;

	for(ap = Aq;ap != NULLAQ;ap = ap->next)
		if (addreq (ap->addr, callsign))
			return ap->iface->name;

	return NULL;
}

/*
 * 14Oct2023, WZ0C, Look up how many seconds ago this station was
 * heard on RF.  Used to know whether or not to gate the packet
 * between APRS-IS and RF.
 */

int aprs_heardrecently( char *callsign )
{
  register struct aq *ap;
  char addr[AXALEN];
  time_t curtime;

  setcall( addr, callsign );
  time(&curtime);
  
  for(ap = Aq;ap != NULLAQ;ap = ap->next)
  {
    if (addreq(ap->addr,addr))
	{
	  /* recent = less than one hour */
	  if( curtime - ap->time <= 3600 )
	    return 1;
	  else if( !dtiheard )
	    /* except here, where DTI's aren't saved */
	    return 0;
	}
  }

  /* Not in list or heard too far back */
  return 0;
}

/*
 * Function : display_aprshrd
 *
 * DISPLAY the APRS Heard table
 *
 * 11Oct2023, New function using the new link listed heard information
 *
 * 15Nov2023, added 3rd argument for better formatting (separation) of
 * the interfaces instead of this mixed interface format from before.
 */

static int show_aprshrdlst (char *dptr, int html, struct iface *ifp)
{
	char tmp[AXBUF], *sptr = dptr;
	register int idx;
	time_t curtime;
	int dspflg;
	char *tptr;

	register struct aq *ap;

	for(ap = Aq;ap != NULLAQ;ap = ap->next)
	{
		if ((ifp && ap->iface == ifp) || !ifp)
		{
			dspflg = 0;

			/* 17Aug2001, Display interface if different from main port */
			/* 15Nov2023, Maiko, Now do only if IFP is null (all interfaces) */
			if (!ifp && strcmp (ap->iface->name, aprs_port))
				dspflg |= 0x01;

			/* 20Aug2001, VE4KLM, Playing with digis */
			if (ap->ndigis)
				dspflg |= 0x02;

			/* 21Aug2001, Lowercase the callsigns (it takes up alot less room)
			 * when viewing an HTML page
			 */
			tptr = strlwr (pax25 (tmp, ap->addr));

#ifdef	APRS_14501

			if (html)
			{
				dptr += sprintf (dptr, "<tr><td>");

				if (locator)
					dptr += sprintf (dptr, "<a href=\"%s%s\">%s</a>", locator, tptr, tptr);
				else
					dptr += sprintf (dptr, "%s", tptr);

				dptr += sprintf (dptr, "</td><td>");

				/* 12Apr2002, Maiko, Cosmetics */
				if (dtiheard)
					dptr += sprintf (dptr, "%c", ap->dti);
				else
					dptr += sprintf (dptr, "n/a");

				dptr += sprintf (dptr, "</td><td>%s</td><td>%d</td><td>",
					tformat ((int32)(time (&curtime) - ap->time)), ap->currxcnt);

				/* 15Nov2023, Maiko, Only do all ifaces if IFP is null */
				if (!ifp && (dspflg & 0x01))
					dptr += sprintf (dptr, "I %s", ap->iface->name);

				if (dspflg & 0x02)
				{
					/* 15Nov2023, Maiko, Only do all ifaces if IFP is null */
					if (!ifp && (dspflg & 0x01))
						dptr += sprintf (dptr, ", ");

					dptr += sprintf (dptr, "P ");

					for (idx = 0; idx < ap->ndigis; idx++)
					{
						if (idx)
							dptr += sprintf (dptr, " ");

					/* 05Sept2001, Maiko, Added '*' repeated flags if present */
						dptr += sprintf (dptr, "%s%s",
							strlwr (pax25 (tmp, ap->digis[idx])),
                    			(ap->digis[idx][ALEN] & REPEATED) ? "*" : "");
					}
				}

				if (!dspflg)
					*dptr++ = '-';

				dptr += sprintf (dptr, "</td></tr>");
			}
			else
#else
			if (1)
#endif
			{
				/* 12Apr2002, Maiko, More cosmetics */
				if (dtiheard)
					tprintf ("%-15.15s%c    ", tptr, ap->dti);
				else
					tprintf ("%-14.14sn/a   ", tptr);

				tprintf ("%s  %-4d  ",
			tformat ((int32)(time (&curtime) - ap->time)), ap->currxcnt);

				/* 15Nov2023, Maiko, Only do all ifaces if IFP is null */
				if (!ifp && (dspflg & 0x01))
					tprintf ("I %s", ap->iface->name);

				if (dspflg & 0x02)
				{
					/* 15Nov2023, Maiko, Only do all ifaces if IFP is null */
					if (!ifp && (dspflg & 0x01))
						tprintf (", ");

					tprintf ("P ");

					for (idx = 0; idx < ap->ndigis; idx++)
					{
						if (idx)
							tprintf (" ");

				/* 05Sept2001, Maiko, Added '*' repeated flags if present */
					tprintf ("%s%s", strlwr (pax25 (tmp, ap->digis[idx])),
                   		(ap->digis[idx][ALEN] & REPEATED) ? "*" : "");
					}
				}

				if (!dspflg)
					tprintf ("-");

				tprintf ("\n");
			}
		}
	}

	if (html)
		return (dptr - sptr);
	else
		return 1;
}

#ifdef	APRS_14501

/* 31Jul2001, VE4KLM, new function for the http 14501 port statistics */
int build_14501_aprshrd (char *dptr)
{
	struct iface *ifp;	/* 15Nov2023, Maiko */

	char *sptr = dptr;

	/* 15Dec2023, Maiko, Don't use maxaprshrd anymore, it's a link list now */
	if (logon_callsign == (char*)0 || aprs_port == (char*)0)
	{
		dptr += sprintf (dptr, "<h4>APRS heard table not configured</h4>");
		return (dptr - sptr);
	}

	/*
	 * 03Jan2001, Maiko, Might as well put this instead of empty table
	 * 13Dec2023, Maiko, used to be topcnt, now link list method.
	 */
	if (numaq == 0)
	{
		dptr += sprintf (dptr, "<h4>No APRS traffic heard so far</h4>");
		return (dptr - sptr);
	}

	for (ifp = Ifaces; ifp->next; ifp = ifp->next)
	{
		if (ifp->type == CL_AX25)
		{
			/* 16Aug2001, Maiko, We now track the port user was heard on */
			dptr += sprintf (dptr, "<h4>APRS Traffic Heard on [%s]</h4><table border=1 cellpadding=3 bgcolor=\"#00ddee\"><tr><td>Callsign</td><td>DTI</td><td>Since</td><td>Pkts</td><td>Path</td></tr>", ifp->name);

			dptr += show_aprshrdlst (dptr, 1, ifp);

			dptr += sprintf (dptr, "</table><br>");
		}
	}
	return (dptr - sptr);
}

#endif

/* this function is the regular aprsstat function, modified 31Jul2001
 * to use above function
 */
void display_aprshrd (void)
{
	struct iface *ifp;	/* 15Nov2023, Maiko */

	/* 15Dec2023, Maiko, Don't use maxaprshrd anymore, it's a link list now */
	if (logon_callsign == (char*)0 || aprs_port == (char*)0)
	{
		tprintf ("\nAPRS heard table not configured\n\n");
		return;
	}

	/*
	 * 03Jan2001, Maiko, Might as well put this instead of empty table
	 * 13Dec2023, Maiko, used to be topcnt, now link list method.
	 */
	if (numaq == 0)
	{
		tprintf ("\nNo APRS traffic heard so far\n\n");
		return;
	}

	for (ifp = Ifaces; ifp->next; ifp = ifp->next)
	{
		if (ifp->type == CL_AX25)
		{
			/* 16Aug2001, Maiko, We now track the port user was heard on */
			tprintf ("\nAPRS traffic heard on [%s]:\nCallsign      DTI     Since     Pkts  Path\n", ifp->name);

			show_aprshrdlst (NULL, 0, ifp);

			tprintf ("\n");
		}
	}
}

/*
 * Fall 2023, Maiko (VE4KLM), replacing original heard list code with
 * the 'standard' ax25 heard list methodology, a better approach, has
 * better sorting functions (by time) and more dynamic, time tested.
 */

struct aq* aq_lookup (ifp, addr, sort, dti)
struct iface *ifp;
char *addr;
int sort;
char dti;
{
	register struct aq *ap;
	struct aq *lqlast = NULLAQ;
  
	for(ap = Aq;ap != NULLAQ;lqlast = ap,ap = ap->next)
	{
		if ((ap->iface == ifp) && addreq(ap->addr,addr))
		{
			if (!dtiheard || (dtiheard && ap->dti == dti))
			{
				if(sort && lqlast != NULLAQ)
				{
					/* Move entry to top of list */
					lqlast->next = ap->next;
					ap->next = Aq;
					Aq = ap;
				}

            	return ap;
			}
        }
    }

    return NULLAQ;
}

struct aq* aq_create(ifp,addr,axload)
struct iface *ifp;
char *addr;
int axload;		/* 16Mar2024, Maiko (VE4KLM), new axheard load flag */
{
    register struct aq *ap;
    struct aq *lqlast = NULLAQ;

	static struct aq *prevAq = NULLAQ;	/* 16Mar2024, Maiko (VE4KLM) */

    if(maxaprshrd && numaq == maxaprshrd) {
        /* find and use last one in list */
	/* 25Apr2023, Maiko, If lp is NULL, this will blow up on the lp->next */
        for(ap = Aq;ap && (ap->next != NULLAQ);lqlast = ap,ap = ap->next);
        /* delete entry from end */
        if(lqlast)
            lqlast->next = NULLAQ;
        else    /* Only one entry, and maxax25heard = 1 ! */
            Aq = NULLAQ;
        ap->currxcnt = 0;
    } else {    /* create a new entry */
        numaq++;
        ap = (struct aq *)callocw(1,sizeof(struct aq));
    }
    memcpy(ap->addr,addr,AXALEN);
    ap->iface = ifp;

	/*
	 * 16Mar2024, Maiko (VE4KLM), I want to get rid of the axhsort functions,
	 * which I wrote back in 2021 for the axheard load functionality. They're
	 * not necessary, all I had to do was change the manner in which the link
	 * list was linked up. So when loading from the axheard file, we simply
	 * just tack each entry to the end of the list, not insert at the top.
	 *
	 * The only pain is that it's done inside the XX_create() functions, so it
	 * looks like I will have to add 'axload' parameter to all the XX_create()
	 * functions, to indicate if new entries are put at the root, or the end.
	 *
	 * This really simplifies axload - not sure what I was thinking back then
	 *  (recall, entries in axheard file are ordered most recent to old)
	 */

	if (axload)
	{
		ap->next = NULLAQ;	/* always for load */

		if (Aq == NULL)
			Aq = prevAq = ap;

		else	/* I think this will work, had to draw it out on paper :) */
		{
			prevAq->next = ap;
			prevAq = ap;
		}
	}
	else	/* default operation is to insert at the very top of link list */
	{
    	ap->next = Aq;
    	Aq = ap;
 	} 
    return ap;
}

/* New function introduced by WZ0C from his first patch */
int aq_remove( addr )
char *addr;
{
  register struct aq *ap;
  struct aq *lqlast = NULLAQ;

  for( ap = Aq; ap != NULLAQ; lqlast = ap,ap = ap->next)
    {
      if( addreq( ap->addr, addr )){
	/* Found it.  Now remove it from the list. */

	if( lqlast )
	  lqlast->next = ap->next;
	else
	  Aq = ap->next;
	ap->next = NULLAQ;
	free( ap );
	numaq--;

	return 1;
      }
    }

  return 0;
}

/*
 * Function : update_aprshrd
 *
 * Update the APRS Heard table
 *
 * 16Aug2001, Maiko, Changed parameters for this function, need more
 * information for statistic purposes and for future enhancements.
 *
 * Fall 2023, Now using new link list calls, old topcnt list gone.
 *
 * 28Sep2023, WZ0C, added 0 option for last.  This signifies that
 * the station was heard via the Internet and should be remove from
 * the "hrd" list.  This has bearing in the calculatings for gating.
 *  (this used to be in the old version of update_aprshd(), but it
 *    is now in located in the new aq_lookup() function above)
 *
 * 15Dec2025, Maiko, Oops, forgot to pass DTI to aq_lookup() ...
 *
 */

void update_aprshrd (struct ax25 *hdr, struct iface *ifp, char dti)
{
	register struct aq *ap;
	time_t curtime;

    if((ap = aq_lookup(ifp,hdr->source,1, dti)) == NULLAQ)
	{
		/* 16Mar2024, Maiko, added 3rd argument to function */
        if((ap = aq_create(ifp,hdr->source, 0)) == NULLAQ)
            return;
	}

	/* standard ax25 heard structure */

    ap->currxcnt++;
    ap->time = time(&curtime);

	/* APRS specific information we want to track */

	if (dtiheard)
		ap->dti = dti;
	else
		ap->dti = ' ';

	ap->ndigis = hdr->ndigis;
	memcpy (ap->digis, hdr->digis, hdr->ndigis * AXALEN);
}

/*
 * 14Mar2024, Maiko (VE4KLM), Save aprs heard list to file as well
 *  (this function called from doaxhsave() in axheard.c module)
 */
int axsaveaprshrd (FILE *fp)
{
	register struct aq *ap;
	char tmp[AXBUF];
	int cnt;

	fprintf (fp, "---\n");
	for (ap = Aq; ap != NULLAQ; ap = ap->next)
	{
		if (ap->iface == NULLIF)
			continue;

		fprintf (fp, "%s %s %d %d %d %d", ap->iface->name,
			pax25 (tmp, ap->addr), ap->time, ap->currxcnt,
				(int)(ap->dti), ap->ndigis);

		for (cnt = 0; cnt < ap->ndigis; cnt++)
			fprintf (fp, " %s", pax25 (tmp, ap->digis[cnt]));

		fprintf (fp, "\n");
	}

	return 0;
}

/*
 * 14Mar2024, Maiko (VE4KLM), Load aprs heard list from file as well
 *  (this function called from doaxhload() in axheard.c module)
 */
int axloadaprshrd (FILE *fp, int gap)
{
	char iobuffer[80], ifacename[30], dti_c;
	char callsign[12], tcall[AXALEN];
	int axetime, count, dti, ndigis;
	struct iface *ifp;
	struct aq *ap;

	while (fgets (iobuffer, sizeof(iobuffer) - 2, fp) != NULLCHAR)
	{
		/*
		 * No need to check for separator, this is the last group for now
		 *
		if (strstr (iobuffer, "---"))
			break;
		 */

		sscanf (iobuffer, "%s %s %d %d %d %d", ifacename, callsign,
			&axetime, &count, &dti, &ndigis);

		dti_c = (char)dti;

		if ((ifp = if_lookup (ifacename)) == NULLIF)
			log (-1, "unable to lookup iface [%s]", ifacename);

		else if (setcall (tcall, callsign) == -1)
			log (-1, "unable to set call [%s]", callsign);

		/* if the call is already there, then don't overwrite it of course */
   		else if ((ap = aq_lookup (ifp, tcall, 1, dti_c)) == NULLAQ)
		{
			/* 16Mar2024, Maiko, added 3rd 'load' argument to function */
			if ((ap = aq_create (ifp, tcall, 1)) == NULLAQ)
				log (-1, "unable to create Aq entry");
			else
			{
				ap->currxcnt = count;

				/*
				 * Only part about having this function in it's own module
				 * which saves me the trouble to export func and structure
				 * definitions, is that I need to pass some timing info.
				 *
				 * 22Mar2024, Maiko, APRS uses time(), not secclock() !
				 */
				ap->time = axetime + gap;

				ap->dti = dti_c;

				/*
				 * 14Mar2024, Maiko, Don't really need to load up the digis,
				 * since it is the main call we really care about. Digis can
				 * change from one packet to another - don't bother for now.
				 */
				ap->ndigis = 0;
			}
		}
	}

	return 0;
}

/*
 * 13Dec2023, Maiko (VE4KLM), the remaining functions below are
 * all new additions courtesty of Michael Ford (WZ0C), taken from
 * the first patch file, 30Oct2023, aprs.patch, support functions
 * he wrote for his ongoing APRS code enhancements ...
 *  (now wondering if the check for time == 0 is required)
 *
 * 14Oct2023 WZ0C - clear_aprshrd removes a callsign from the
 * heard list, normally because it was heard on the Internet
 * instead of RF.  We need to know where it was heard last to
 * know whether to gate between APRS-IS and RF.
 */

void clear_aprshrd( char *callsign )
{
  char addr[AXALEN];

  setcall( addr, callsign );

  aq_remove( addr );

  return;
}

/* type isn't defined yet.  We're only using this to signify a station
 * is an igate. */

void aprs_setstatus( char *callsign, int type )
{
  char filename[100];
  time_t curtime;

  FILE *fp;

  sprintf (filename, "%s/db/types/%s", APRSdir, callsign);

  if ((fp = fopen (filename, "w")) != NULLFILE)
    {
      fprintf (fp, "%ld igate", time(&curtime));
      
      fclose (fp);
    }

  return;
}

int aprs_getstatus( char *callsign )
{
  time_t when, curtime;
  FILE *fp;
  char buf[100], *cp;
  char filename[100];

  sprintf (filename, "%s/db/types/%s", APRSdir, callsign);

  if ((fp = fopen (filename, "r")) != NULLFILE)
    {
      fgets( buf, 99, fp );

      cp = strchr( buf, ' ' );
      if( cp == NULL )
	return 0;
      
      *cp = '\0';
      cp++;

      when = atol( buf );
      time(&curtime);

      fclose (fp);

      if( curtime - when < 86400 )
	return 1;
    }

  return 0;
}

void aprs_addstatexception( char *callsign )
{
  char filename[100];
  time_t curtime;

  FILE *fp;

  sprintf (filename, "%s/db/exceptions/%s", APRSdir, callsign);

  if ((fp = fopen (filename, "w")) != NULLFILE)
    {
      fprintf (fp, "%ld", time(&curtime));
      
      fclose (fp);
    }

  return;
}

int aprs_hasstatexception( char *callsign )
{
  char buf[100], filename[100];
  time_t when, curtime;
  FILE *fp;

  sprintf (filename, "%s/db/exceptions/%s", APRSdir, callsign);

  if ((fp = fopen (filename, "r")) != NULLFILE)
    {
      fgets( buf, 99, fp );

      when = atol( buf );
      time(&curtime);

      fclose (fp);

      unlink( filename );

      if( curtime - when < 900 )
	return 1;
    }

  return 0;
}

#endif

