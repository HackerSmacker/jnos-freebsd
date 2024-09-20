#ifndef	_BBS10000_H
#define _BBS10000_H

/*
 * 05Nov2020, Maiko, Convert from GET to POST when submitting forms, and
 * other refinements, added 'abort_char' to added HTTPVNCSESS structure.
 *
 * Web Based BBS Access for JNOS 2.0g and beyond ...
 *
 *   June 8, 2009 - started working in a play area on a work machine.
 *   July 3, 2009 - put into my official development source (prototype).
 *  July 21, 2009 - prototype pretty much ready for single web client.
 * February, 2010 - refining session control for multiple web clients at once.
 * April 24, 2010 - seems to be working nicely over last while - done.
 *
 * Designed and written by VE4KLM, Maiko Langelaar
 *
 * For non-commercial use (Ham Radio) only !!!
 *
 */

typedef struct {

    char *name;
    char *pass;

} PASSINFO;

/*
 * 08July2009, Maiko (VE4KLM), Need session tracking so that we can have
 * multiple HTTP VNC connections going at the same time, for different calls
 * and from different clients, etc.
 *
 * 06Oct2023, Maiko, First attempt at IPV6 support
 */

typedef struct {

	int32 ipaddr;
#ifdef	IPV6
	unsigned char ipaddr6[16];
#endif
	char *currcall;
	int mbxs[2];
	int escape_char;
	int clear_char;
	int mbxl;
	int mbxr;

	PASSINFO passinfo;

	//char tfn[20];	/* give each CALL their own session web file */
	char *tfn;	/* 09Aug2022, VE4KLM (Maiko), now a pointer */

	FILE *fpsf;	/* Session File write file descriptor */

	int valid;	/* 23Feb2010, session validation and port attacks */

	int reused;	/* 06Mar2010, track number of times slot is reclaimed */

	int abort_char;	/* 05Nov2020, Maiko, new to abort sending message */

	/*
	 * 04Sep2022, Maiko, new to send message, although I wonder if we really
	 * need this, since we already have '/ex' to use at the bbs prompt. Just
	 * trying to be 'complete' I guess ? Perhaps this is not useful ...
	 */
	int send_char;

} HTTPVNCSESS;

#endif

