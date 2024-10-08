#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * JNOS 2.0 Default configuration file if starting from scratch
 *
 * December 3, 2020, Maiko (VE4KLM), trying to clean this up :/
 *  (it's not working very well, this file is just crazy)
 */

/* Definitions introduced with JNOS 2.0o - 03Mar2023, Maiko */

#define	KLM_MAILIT_MIMEAWARE	/* mime processing, local mail */

#define	LOWER_CASE_BID_KLUDGE	/* for Charles, N2NOV, and others maybe ? */

/*
 * Definitions introduced with JNOS 2.0m.5F
 */

#define EA5HVK_VARA     /* Supporting the VARA modem - Jan 5, 2021
			 * huge mods, enhancments by WZ0C, Michael Ford (2023)
			 * requires #define PPP much further (done already)
			 */

/* #undef REMOTE_RTS_PTT  New 28Jan2021, removed 29Aug2023, Maiko, VE4KLM */

#undef J2_DONT_ENFORCE_BBS_USER		/* define if this gives you a headache */

#undef TNODE	/* Brian's TNODE mods */

/*
 * Definitions introduced with JNOS 2.0m.5E
 */

#undef	J2MFA		/* Multi Factor Authentication (MFA) prototype */

#define	WINRPR		/* KISS over TCP/IP interface for SCS WinRPR software */

/*
 * Definitions introduced with JNOS 2.0m.3, 2.0n (shelved for now), and earlier ?
 */

#define	BACKUP_AXHEARD	/* support for saving and loading axheard list */

#define	TNCALL		/* support Packet Winlink connects, just like an RMS node  */

/*
 * Decided and documented around 5 to 5:30 pm - October 12, 2019
 *
 * lzhuf.c - code added for B2F is actually used for B1F, please refer
 * to that source file for further clarification and documentation on
 * the '#define B2F' directive.
 *
 * NEW RULE - #define B2F is now a PERMANENT / NOT TO BE CHANGED parameter !
 *
 * IF folks do not want Winlink Secure Login, which necessitates installation
 * of the openssl development package for linux, then #undef the NEW compiler
 * directive below instead, that way there is no risk to the forwarding code.
 */

#define WINLINK_SECURE_LOGIN       /* support W2LK secure login */

/* Feb2017, Maiko (VE4KLM), new defines for 2.0k.1 */

/*
 * 04Feb2017, Maiko (VE4KLM), Add support for CAPA and UIDL features to allow
 * email clients like thunderbird to 'leave messages on server' and so on ...
 */
#define J2_CAPA_UIDL

/*
 * 26Oct2016, Maiko (VE4KLM), The JNOS smtp client is limiting in that if one
 * sends an email to a main recipient and multiple carbon copy recipients, all
 * going to the same destination host, then delivery of carbon copy recipients
 * is delayed till the next SMTP tick, and so on. This scenario was reported
 * by N6MEF (Fox), so I've made small changes to allow multiple SMTP sessions
 * to exist to the same destination box, which should solve this problem.
 *
 * BE CAREFUL - do you want multiple sessions to same host on slow links ?
 */
#define J2_MULTI_SMTP_SESSION

/* Jun2016, Maiko (VE4KLM), new defines since 2.0j.7u */

#define	JNOS_DEFAULT_NOIAC  /* preserve original JNOS default, leave it ! */

#define JNOS_LOG_INCOMING_LOCAL_PORTS	/* experimental port logging */

#define J2_SID_CAPTURE	/* capturing and logging of SID information */

#undef MAIL_LOG_EXTRAS	/* 10Mar2016, include subject and msgid in mail.log */

/* end of additions */

#define JNOS_LOG_CONSOLE	/* 14May2015, Maiko, log F10/console commands */

#define	FLDIGI_KISS	/* 16May2015, Maiko (VE4KLM), FLDIGI KISS interface */

/*
 * 25Feb2015, Maiko (VE4KLM), Instead of telling me that a particular console
 * command 'takes at least one argument', quit wasting my time and just give me
 * a list of possible arguments. This was driving me nuts for a long time ...
 */
#define SUBCMD_WASTEFULL_PROMPTING

/*
 * 02Feb2015, Maiko (VE4KLM), Very simple for starters, need to give this
 * a lot more thought down the road. Any callcheck failures in mailbox.c,
 * are automatically added to the TCP ACCESS DENY list. Note, 'tcp access'
 * command has been revamped a bit, you can now 'insert' into the list.
 */
#define	BLACKLIST_BAD_LOGINS

#define	TCPACCESS_EXPIRY	/* 15Feb2015, Maiko (VE4KLM), leave defined */

/*
 * 16Nov2014, Maiko (VE4KLM), Very experimental feature to mark unreachable
 * nodes, such that they do not appear in node listings, users will not be able
 * to connect or route to them. They will stay in the node tables, but ignored,
 * till such time they become reachable again (determined by sysop command).
 *
 * 10Mar2015, Maiko (VE4KLM), Not ready at all, ie, don't use it yet
 */
#undef J2_NETROM_REACHABILITY

#define	AGWPE		/* 23Feb2012, Maiko, Begin AGWPE tcp/ip interface */

#define WPAGES		/* 01Feb2012 - incorporate Lantz TNOS WP code */

#define J2_TRACESYNC	/* 04Nov2010 (New) - trace files are now syncable */

#undef HEADLESS		/* 07Nov2010 (New) - no console, no curses */

#define	APRSD		/* APRS Igate and other services */
#define	JNOSAPRS	/* Mandatory define for JNOS platforms */
#undef APRSC		/* define this if all you want is an APRS client */

#define RIPAMPRGW	/* 20Feb2010 - special code for AMPRgw RIP updates */

/*
 * 04Aug2013 - Gus (I0OJJ) reported getting RIP broadcasts for which the tag
 * field was NOT being set, which effectively caused JNOS to ignore incoming
 * RIP information, so one can now #define this option to fix the situation.
 */
#undef IGNORE_RIP_TAG

#define TUN		/* Use TUN linux kernel module for networking */
#define HFDD		/* HFDD (HF Digital Devices) - Pactor Host Mode */
#define DYNGWROUTES	/* Enable use of dynamic gateways in 'route add' */
#define NRR		/* NRR (Netrom Route Record) feature */

/*
 * 12Oct2019, VERY IMPORTANT - the B2F directive is now permanent !
 *
 * Do NOT #undef this under any circumstances !!!
 *
 * IF you want Winlink Secure Login, then use WINLINK_SECURE_LOGIN
 *  (just search for it, it's near the top of this file)
 *
 */
#define B2F		/* Support for B2F forwarding */

/*
 * I have not looked @ INP3 for a long time, it really needs a
 * revamp, and to be honest, when I first put this together my
 * understanding of the principles behind INP3 vs NETROM was
 * quite limited, it still is ... 04Dec2020, Maiko (VE4KLM)
 *
 * That's why it is now undefined by default, and some people
 * have reported stability (crash) issues regarding its use.
 */
#undef	INP2011		/* New INP (read only for now) support (Sep2011) */

#define JNOSPTY98	/* Better support for /dev/ptmx, /dev/pts/N (Dec2011) */

#undef WELCOMEUSERS		/* Extraneous welcome info */
#undef DEFAULT_SIGNATURE	/* Default signature for all mail */

#define MAIL_HDR_TRACE_USER_PORT	/* Put User Port in mail headers */

#undef AX25_XDIGI	/* User customizable cross port digi rules */
#undef SYSEVENT		/* Execute shell scripts for certain events */
 
#define	MBX_CALLCHECK	/* Callsign (BBS) validation */ 
#define SDR_EXCEPTIONS	/* SMTP Deny Relay exceptions */
#define SGW_EXCEPTIONS	/* SMTP Gateway exceptions (new for Dec2011) */

/*
 * Jun2016, Maiko (VE4KLM), No longer exists as of version 2.0k onwards ...
 *
 * Telpac and early XFBB uses of CR line endings
 *
#define	MB_XFBB_KLUDGE
 */

#undef	BAYCOM_SER_FDX	/* Support for Baycom Board kernel driver */
#define	MULTIPSK	/* Support for MultiPSK tcp/ip server */

#define HTTPVNC		/* NEW (03Jul09) - Browser based VNC for JNOS */

#define NOINVSSID_CONN	/* NEW (09Aug10) - Connect without SSID invert */

#define JNOS20_SOCKADDR_AX  /* NEW (30Aug10) - Permanent do NOT undefine */

/* NEW (19Oct10) Bob (K6FSH) Site Specific Directives - see 'readme.k6fsh' */

#undef MBX_TAC_CALLCHECK
#define MBX_AREA_PROMPT
#define MBX_MORE_PROMPT
#undef LISTING_ISNT_READING

#define WINMOR	/* NEW (Early 2010) - WINMOR Sound Card TNC (over tcp/ip) */

#define SNMPD	/* NEW (17Dec2010) - SNMPD (for my mrtg for starters) */

#define J2_SNMPD_VARS	/* 31Jan2011 - Permanent do NOT undefine */

/*
 * Section 2 - The original JNOS 1.11f configuration options.
 */

/* This is the configuration as distributed by WG7J */

/* #define WHOFOR "description_goes_here" */
  
/* NOTE: only the below listed config files have been tested.
 * Due to the virtually unlimited number of combinations of options
 * in config.h, it is impossible to test each possible variation!
 * Others may or may not compile and link without errors !
 * Effort has been made to provide a clean set of #defines throughout
 * the source to produce a good compile, but no guarantees are made.
 * Your mileage may vary!!!
 * tested are: distconf.h, gwconfig.h, bbsconf.h, users.h homeslip.h
 */
  
/* Software options */
  
#define CONVERS		/* Conference bridge (babble-box :-) */

/* Now some converse options ... see convers.c for more comments */
#define LINK		/* permit this convers node to be linked with others*/
#undef XCONVERS		/* LZW Compressed convers server and links */
#undef CNV_VERBOSE	/* Verbose msgs */
#define CNV_CHAN_NAMES	/* Convers named channels */
#define CNV_TOPICS	/* Convers channel topics are gathered */
#undef CNV_CALLCHECK	/* Convers only allows callsigns */
#undef CNV_LOCAL_CHANS	/* Convers local channels and msg-only channels */
#define CNV_ENHANCED_VIA  /* If convers user is local, "via" gives more info */
#undef CNV_CHG_PERSONAL	/* Allow users to change personal data permanently */
#define CNV_LINKCHG_MSG	/* Send link-change messages in convers */
#undef CNV_VERBOSE_CHGS	/* Default to /VERBOSE yes. Use this judiciously! */
#undef CNV_TIMESTAMP_MSG /* Add hh:mm prefix to msgs sent to local users */

/* Use only ONE of the 2 news options: */
#undef NNTP		/* Netnews client */
#undef NNTPS		/* Netnews client and server */
#define NEWS_TO_MAIL	/* NNTPS emails per gateway file */
#define NN_USESTAT	/* Try GROUP/STAT cmds if NEWNEWS fails */
#undef NN_INN_COMPAT	/* send "mode reader" cmd after connecting to server */
#define NN_REMOVE_R_LINES	/* remove R: lines from incoming email */
#define NNTP_TIMEOUT	900 /* idle-timeout #secs for nntp client (both versions)*/
#define NNTPS_TIMEOUT	3600 /* idle-timeout #secs for nntp server */

#undef STKTRACE		/* Include stack tracing code */
#define TRACE		/* Include packet tracing code */
#define MONITOR		/* Include user-port monitor trace mode */
#undef MONSTAMP		/* add time stamp to monitor-style trace headers */
#undef DIALER		/* SLIP/PPP redial code */
#undef POP2CLIENT	/* POP2 client -- IAB not recommended */
#define POP3CLIENT	/* POP3 client -- IAB draft standard */
#undef POPT4		/* add 'pop t4' command to pop3 client, setting timeout */
#undef RDATECLI		/* Time Protocol client */
#define REMOTECLI	/* remote UDP kick/exit/reset */
#undef ESCAPE		/* Allow Unix style escape on PC */
#define ATCMD		/* Include timed 'at' execution */
#define NR4TDISC	/* Include Netrom L4 timeout-disconnect */
#undef XMODEM		/* xmodem file xfer for tipmail  */
#undef IPACCESS		/* Include IP access control code */
#define TCPACCESS	/* Include TCP access control code */
#undef MD5AUTHENTICATE	/* Accept MD5-Authenticated logins */
#define ENCAP		/* Include IP encapsulation code */
#undef MBOX_DYNIPROUTE	/* Add XG mbox cmd to route dynamic IPaddr via encap*/
#define UDP_DYNIPROUTE	/* Support dynamic IPaddr encap routes via UDP/remote */
#undef AUTOROUTE	/* Include AX.25 IP auto-route code(causes problems when VC mode is used for ip) */
#undef HOPPER		/* Include SMTP hopper code by G8FSL */
#undef TRANSLATEFROM	/* smtp server rewrites from addrs too */
#undef SMTP_REFILE	/* smtp server rewrites to addr according to from|to */
#undef AGGRESSIVE_GCOLLECT /* exit 251 when availmem < 1/4 of 'mem threshold'*/
#undef KICK_SMTP_AFTER_SHELLCMD /* kick smtp client after each shell cmd */
#undef TN_KK6JQ		/* add more telnet options support */
#define LOCK		/* Include keyboard locking */
#undef AX25PASSWORD	/* Ask ax.25 users for their passwords */
#undef NRPASSWORD	/* Also ask NetRom users for passwords */
#define TTYCALL		/* Include AX.25 ttylink call */
#undef TTYCALL_CONNECT	/* SABM pkt uses TTYCALL, not BBSCALL, as src call */
#undef MULTITASK	/* Include Dos shell multi-tasker */
#define SHELL		/* Include shell command */
#undef SWATCH		/* stopWATCH code */
#undef UNIX_DIR_LIST	/* Unix-style output from DIR and ftp DIR cmds. */
#undef MEMLOG		/* include alloc/free debugging to MEMLOG.DAT file? */
#define ALLCMD		/* include dump,fkey,info,mail,motd,record,tail,
			 * taillog,upload,watch commands */
#define DOSCMD		/* Include cd,copy,del,mkdir,pwd,ren,rmdir commands */
#define MAILMSG		/* Include mailmsg command */
  
#define SPLITSCREEN	/* Needed for split, netrom split, and ttylink cmds */
#define STATUSWIN	/* Up to 3 line status window */
#undef STATUSWINCMD	/* status off|on command to modify status window */
#undef SHOWIDLE		/* show relative system-idle in status line */
#undef SHOWFH		/* show free filehandles in status line */

#undef PS_TRACEBACK	/* ps <pid> option enabled to do a back-trace */
#define RXECHO		/* Echo rx packet to another iface - WG7J */
#define REDIRECT	/* Allow cmd [options] > outfile. Use >> to append */
#undef EDITOR		/* include internal ascii editor */
			/* PICK ONE FROM CHOICES ED or TED: */
#define ED		/* editor uses Unix ed syntax; OK for remote sysops. ~13KB */
#undef TED		/* editor uses TED syntax; local console only */
  
/* Protocol options */
  
#define AX25		/* Ax.25 support */
#define NETROM		/* NET/ROM network support */
#define NRS		/* NET/ROM async interface */
#define RIP		/* Include RIP routing */
#undef RIP98		/* Include RIP98 routing */
#define LZW		/* LZW-compressed sockets */
#define SLIP		/* Serial line IP on built-in ports */
#define PPP		/* Point-to-Point Protocol code */
#undef PPP_DEBUG_RAW	/* Additional PPP debugging code...see pppfsm.h */
#undef VJCOMPRESS	/* Van Jacobson TCP compression for SLIP */
#undef RSPF		/* Include Radio Shortest Path First Protocol */
#define AXIP		/* digipeater via ip port 93 interface */
#undef RARP		/* Include Reverse Address Resolution Protocol */
#undef BOOTPCLIENT	/* Include BootP protocol client */
  
  
/* Network services */
  
#define SERVERS		/* Include TCP servers */
#define AX25SERVER	/* Ax.25 server */
#define NETROMSERVER	/* Net/rom server */
#define TELNETSERVER	/* Tcp telnet server */
#undef RSYSOPSERVER	/* Tcp telnet-to-mbox-as-sysop server */
#undef TRACESERVER	/* remote interface trace server */
#define TTYLINKSERVER	/* Tcp ttylink server */
#undef TTYLINK_AUTOSWAP	/* ttylink server automatically swaps to new session */
#define SMTPSERVER	/* Tcp smtp server */
#undef RELIABLE_SMTP_BUT_SLOW_ACK /* smtp server delays msg ack until filing completed */
#undef TIMEZONE_OFFSET_NOT_NAME /* smtp headers use hhmm offset from GMT */
#define SMTP_DENY_RELAY	/* Refuse to relay msgs from hosts not in our subnets */
#undef SMTP_VALIDATE_LOCAL_USERS /* local user must be in ftpusers/popusers/users.dat */
#define FTPSERVER	/* Tcp ftp server */
#undef FTPDATA_TIMEOUT	/* ftp server timeout on recvfile */
#undef WS_FTP_KLUDGE	/* ftp server lies to please ws_ftp winsock app */
#undef FTPSERV_MORE	/* ftp server supports RNFR, RNTO, MDTM commands */
#define FINGERSERVER	/* Tcp finger server */
#undef POP2SERVER	/* POP2 server -- IAB not recommended */
#define POP3SERVER	/* POP3 server -- IAB draft standard */
#define POP_TIMEOUT	600 /* pop server idle timeout value, in secs (>= 600) */
#define REMOTESERVER	/* Udp remote server */
#undef RDATESERVER	/* Time Protocol server */
#undef ECHOSERVER	/* Tcp echo server */
#undef DISCARDSERVER	/* Tcp discard server */
#define TIPSERVER	/* Serial port tip server */
#define DOMAINSERVER	/* Udp Domain Name Server */
#undef TERMSERVER       /* Async serial interface server */
#undef IDENTSERVER	/* RFC 1413 identification server (113/tcp) */
#undef BOOTPSERVER	/* Include BootP protocol server */
#undef TCPGATE		/* TCPGATE redirector server */
/* Pick ONE of the following 5 callbook servers: */
#undef CALLSERVER	/* Include BuckMaster CDROM server support */
#undef ICALL		/* Buckmaster's international callsign database April '92 */
#undef BUCKTSR		/* Buckmaster callsign DB via bucktsr.exe (April 95) */
#undef SAMCALLB		/* SAM callbook server. Note that you can NOT have */
			/* BOTH Buckmaster and SAM defined.If so, SAM is used */
#undef QRZCALLB		/* QRZ callbook server. Note that you can NOT have */
			/* BOTH Buckmaster and QRZ defined.If so, QRZ is used */

#define HTTP		/* Selcuk Ozturk's HTTP server on port 80 */
#define HTTP_EXTLOG	/* HTTP: Add detailed access logging in "/wwwlogs" dir */
#undef CGI_XBM_COUNTER	/* HTTP: Add an X-bitmap counter via CGI */
#undef CGI_POSTLOG	/* HTTP: Add a POST demo via CGI */
  
/* Outgoing Sessions */
  
#define SESSIONS

#undef CALLCLI		/* Include only callbook client code (to query remote server) */
#define AX25SESSION	/* Connect and (if SPLITSCREEN) split commands */
#undef AXUISESSION	/* Ax.25 unproto (axui) command */
#define NETROMSESSION	/* netrom connect & split iff NETROM defined */
#define TELNETSESSION	/* telnet cmd */
#define TTYLINKSESSION	/* ttylink cmd - split-screen chat */
#define BBSSESSION	/* bbs (same as telnet localhost) */
#define FTPSESSION	/* ftp,abort,ftype, and iff LZW: ftpclzw,ftpslzw cmds */
#define FTP_RESUME	/* add Jnos ftp client resume&rput cmds */
#define FTP_REGET	/* add RFC959 ftp client reget&restart cmds */
#define FTP_RENAME	/* add RFC959 ftp client rename command */
#define FINGERSESSION	/* finger cmd */
#define PINGSESSION	/* ping cmd */
#define HOPCHECKSESSION	/* IP path tracing command */
#undef RLOGINSESSION	/* Rlogin client code */
#define TIPSESSION	/* tip - async dumb terminal emulator */
#define DIRSESSION	/* dir cmd */
#define MORESESSION	/* more - view ASCII file page by page */
#define REPEATSESSION	/* repeat cmd */
#define LOOKSESSION	/* follow user activity on the bbs */
#define DQUERYSESSION	/* Include "domain query" cmd */
  
/* Mailbox options */
  
#define MAILBOX		/* Include SM0RGV mailbox server */
#define MAILCMDS	/* Include mail commands, S, R, V etc */
#undef SEND_EDIT_OK	/* Send cmd offers (E)dit option to mbox users */
#define FILECMDS	/* Include D,U,W,Z commands */
#define GATECMDS	/* Include gateway releated commands C,E,N,NR,P,PI,T */
#define GWTRACE		/* Log all gateway connects to the logfile */
#define FOQ_CMDS	/* Include Finger, Operator, Query
			 * If GATECMDS and FOQ_CMDS are both undefined,
			 * extra code is saved! */
#undef MBOX_PING_ALLOWED /* undef=>telnet permission needed for mbox ping */
#undef MBOX_FINGER_ALLOWED /* undef=>telnet permission needed for mbox finger */

#define EXPIRY		/* Include message and bid expiry */
#define MAILFOR		/* Include Mailbox 'Mail for' beacon */
#define RLINE		/* Include BBS R:-line interpretation code */
#define MBFWD		/* Include Mailbox AX.25 forwarding */
#define FBBFWD		/* add enhanced FBB Forwarding code (no compression) */
#define FBBCMP		/* add FBB LZH-Compressed forwarding code */
#undef FWDCTLZ		/* Use a CTRL-Z instead of /EX to end message forwarding */
#undef FWD_COMPLETION_CMD /* run a forwarding-completed command if set in script */
#define FBBVERBOSELOG	/* log more data for FBB-protocol transfers */
#define USERLOG		/* Include last-msg-read,prompt-type user tracking */
#define REGISTER	/* Include User Registration option */
#undef MAILERROR	/* Include Mail-on-error option */
#undef HOLD_LOCAL_MSGS	/* Hold locally-originated msgs for review by sysop */
#undef FWDFILE		/* Include forwarding-to-file (export) feature */
#define EXPEDITE_BBS_CMD /* Use MD5 and net.rc to autologin console to bbs */
  
  
/* Memory options */
  
#undef EMS		/* Include Expanded Memory Usage */
#undef XMS		/* Include Extended Memory Usage */
  
  
/* Software tuning parameters */
  
#define MTHRESH		16384	/* Default memory threshold */
#define NROWS		25	/* Number of rows on screen */
#define NIBUFS		5	/* Number of interrupt buffers */
#define IBUFSIZE	2048	/* Size of interrupt buffers */
#define NSESSIONS	10	/* Number of interactive clients */
#define NAX25		24	/* Number of axip interfaces (if defined) */
#define MAXSCC		4	/* Max number of SCC+ESCC chips (< 16) */
  

/* Hardware driver options */
  
#define ASY		/* Asynch driver code */
#undef HP95		/* hp95-style uart handling */
#define KISS		/* Multidrop KISS TNC code for Multiport tnc */
#undef POLLEDKISS	/* G8BPQ Polled Multidrop KISS TNC code */
#undef PACKET		/* FTP Software's Packet Driver interface */
#undef SCC		/* PE1CHL generic scc driver */
#undef BPQ		/* include Bpqhost interface */
#undef PACKETWIN	/* Gracilis PackeTwin driver */
#undef PI		/* VE3IFB pi dma card scc driver */
#undef ARCNET		/* ARCnet via PACKET driver */
#undef PC_EC		/* 3-Com 3C501 Ethernet controller */
#undef HS		/* High speed (56kbps) modem driver */
#undef HAPN		/* Hamilton Area Packet Network driver code */
#undef EAGLE		/* Eagle card driver */
#undef PC100		/* PAC-COM PC-100 driver code */
#undef APPLETALK	/* Appletalk interface (Macintosh) */
#undef DRSI		/* DRSI PCPA slow-speed driver */
#undef SLFP		/* SLFP packet driver class supported */
#undef PRINTEROK	/* OK to name a printer as an output device */
  
  
/***************************************************************************/
/* This section corrects some defines that include/exclude others          */
/* You should not normally change anything below this line.                */

/* 19Oct2010, Maiko (VE4KLM), Callcheck might be used in other modules */
#ifndef MBX_CALLCHECK
#if defined(APRSD) || defined(HTTPVNC)
#define MBX_CALLCHECK
#endif
#endif

#ifdef BLACKLIST_BAD_LOGINS
#define TCPACCESS
#endif

#ifdef STATUSWIN
#define SPLITSCREEN
#endif

/* 07Nov2010, Maiko, Headless requires us to undefine a few others ! */
#ifdef HEADLESS
#undef SPLITSCREEN
#undef STATUSWIN
#undef STATUSWINCMD
#undef LOCK
#undef REPEATSESSION
#endif

#ifdef DIRSESSION
#undef MORESESSION
#define MORESESSION
#endif
  
#if defined(NRS)
#undef  NETROM
#define NETROM	/* NRS implies NETROM */
#endif
  
#if defined(ARCNET) || defined(SLFP)
#undef  PACKET
#define PACKET	/* FTP Software's Packet Driver interface */
#endif
  
#if defined(PC_EC) || defined(PACKET)
#define ETHER	/* Generic Ethernet code */
#endif
  
#if defined(BUCKTSR)
#define CALLCLI
#undef CALLSERVER
#undef ICALL
#undef SAMCALLB
#undef QRZCALLB
#endif

#if defined(SAMCALLB)
#define CALLCLI
#undef CALLSERVER
#undef ICALL
#undef BUCKTSR
#undef QRZCALLB
#endif
  
#if defined(QRZCALLB)
#define CALLCLI
#undef CALLSERVER
#undef ICALL
#undef BUCKTSR
#undef SAMCALLB
#endif
  
#if defined(CALLSERVER)
#define CALLCLI
#endif
  
#if defined(POP2CLIENT) || defined(POP3CLIENT)
#define MAILCLIENT
#endif
  
#ifdef POLLEDKISS
#define KISS
#endif
  
#ifndef TRACE
#undef MONITOR
#undef MONSTAMP
#undef TRACESERVER
#endif

#ifndef MAILBOX
#undef MAILCMDS
#undef FILECMDS
#undef GATECMDS
#undef FOQ_CMDS
#undef CALLBOOK
#undef CALLCLI
#undef EXPIRY
#undef MBXTDISC
#undef TIPSERVER
#undef MAILFOR
#undef RLINE
#undef MBFWD
#undef FBBFWD
#undef FBBCMP
#undef USERLOG
#endif
  
#ifndef MAILCMDS
#undef MAILFOR
#undef RLINE
#undef MBFWD
#undef HOLD_LOCAL_MSGS
#endif
  
#ifndef TIPSERVER
#undef XMODEM
#endif
  
#ifndef AX25
#undef AX25SESSION
#undef AXUISESSION
#undef AX25SERVER
#undef MAILFOR
#undef NRS
#undef NETROM
#undef NETROMSESSION
#undef NETROMSERVER
#undef AXIP
#undef NR4TDISC
#undef TTYCALL
#undef BPQ
#undef EAGLE
#undef SCC
#undef KISS
#undef POLLEDKISS
#undef HAPN
#undef PI
#undef PC100
#undef HS
#undef AX25PASSWORD
#undef NRPASSWORD
#endif
  
#ifndef NETROM
#undef NRPASSWORD
#undef NETROMSESSION
#undef NETROMSERVER
#endif
  
#ifndef SMTPSERVER
#undef MAILCMDS
#undef MBFWD
#undef MAILFOR
#undef RLINE
#undef TRANSLATEFROM
#endif
  
#ifndef MBFWD
#undef FWDFILE
#undef FBBFWD
#undef FBBCMP
#endif
  
#ifndef FBBFWD
#undef FBBCMP
#else
#define USERLOG	/* need to remember (new)lastread */
#endif

#ifndef CONVERS
#undef LINK
#undef XCONVERS
#undef CNV_VERBOSE
#undef CNV_CHAN_NAMES
#undef CNV_TOPICS
#undef CNV_CALLCHECK
#undef CNV_LOCAL_CHANS
#undef CNV_ENHANCED_VIA
#undef CNV_CHG_PERSONAL
#undef CNV_LINKCHG_MSG
#undef CNV_VERBOSE_CHGS
#endif

#ifndef LZW
#undef XCONVERS
#endif

#ifdef UNIX
/* Following options are incompatible with UNIX environment */
/* Many go away when MSDOS is not defined, but it doesn't hurt to undef them here too */
#undef SHOWIDLE
#undef MULTITASK
#undef MEMLOG
#undef TED
#undef EMS
#undef XMS
#undef PACKET
#undef STKTRACE
#undef SWATCH
#undef SCC
#undef PI
#undef BPQ
#undef HS
#undef HAPN
#undef EAGLE
#undef DRSI
#undef PC100
#undef SLFP
#undef PACKETWIN
#undef ARCNET
#undef PC_EC
/* what else?? */
/* Following might work someday, but need work... */
#undef TERMSERVER
#undef CALLSERVER
#undef ICALL
#undef BUCKTSR
#undef SAMCALLB
/* be sure we have a session mgr */
#ifndef SM_CURSES
#ifndef SM_DUMB
#ifndef SM_RAW
#define SM_CURSES
#endif
#endif
#endif
#else /* ! UNIX */
#undef SM_CURSES
#undef SM_DUMB
#undef SM_RAW
#endif

#if defined(EDITOR) && defined(ED) && defined(TED)
#error Cannot #define both ED and TED
#endif
#if defined(NNTP) && defined(NNTPS)
#error Cannot #define both NNTP and NNTPS
#endif
#if NIBUFS == 0
#error NIBUFS should never be zero
#endif

#endif  /* _CONFIG_H */
