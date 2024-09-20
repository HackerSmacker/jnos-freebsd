#ifndef J2_DEBUG_H
#define J2_DEBUG_H

/*
 * New centralized JNOS 2.0 debug management code
 *
 * 02Feb2024, Maiko (VE4KLM)
 */

#define	NUM_J2DEBUG_FLAGS 1

/* just one flag for starters */

#define J2DEBUG_LZHUF	0

extern int j2debugflags[NUM_J2DEBUG_FLAGS];

extern int doj2debug (int argc, char **argv, void *p);

extern int j2debug (int which);

#endif
