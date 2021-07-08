/* CmdTool.cpp: Command Interface Test Tool */

/* Copyright (c) 1995-2010,2015, Jet Propulsion Laboratory */

#ifdef MODULE_HDR
/* ***************************************************************************
*
* Project:      Thirty Meter Telescope (TMT)
* System:       Primary Mirror Control System (M1CS)
* Task:         Lower Segment Box
* Module:       CmdTool.cpp
* Title:        LSEB Command Interface Test Tool
* ----------------------------------------------------------------------------
* Revision History:
* 
*   Date           By               Description
* 
*  08-20-05       gbrack            Initial Delivery (for SCDU project)
*  07-01-21       gbrack            Modified for testing M1CS LSEB Interface
* ----------------------------------------------------------------------------
* 
* Description:
*	This is a simple demo program to test sending commands over the
*       network to the LSEB's command interface. This program allows the user 
*       to enter commands from the command line, then formats them into 
*       the correct CmdMsg structure and sends it to the command processor.
*       It then receives a RspMsg back from the command processor and 
*       displays the result.
*       
* Functions Defined:
*       void main()      -- main program (initializes connection to sequencer  
*       void main_loop() -- infinite loop reading standard input, sending
*	                    and receiving data from the sequencer.
*       int send_cmd()   -- send CmdStr to server connected to fd.
*************************************************************************** */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include "net.h"
#include "net_m1cs.h"
#include "GlcMsg.h"
#include "GlcLsebIf.h"


void main_loop();
int send_cmd (int fd, char *cmdStr, char **error);

static char Invalid_Parameters[] =  "REJECT:  INVALID PARAMETERS";
static char Error_Sending[] =  "REJECT:  ERROR SENDING COMMAND";
static char server[] = LSEB_CMD_SRV;
static int  pname = LSEB_CMD_TASK;
static char hostname[64];
static char last_cmd[256] = "";
static int  quitting = 0;
static int  debug = FALSE;
static int  seqNo = 0;                 //!< Command sequence number.

int msgfd;

static CmdMsg msg = {
    { CMD_TYPE, ANY_TASK, 0, sizeof(msg) },  //!< Anonymous client 
    ""
};


int main (int argc, char *argv[]) {
    int i;

    for ( i = 1; i < argc; i++ ) {
        if (!strcmp (argv[i], "-h"))
	    (void) strcpy (hostname, argv[++i]);
	else if (!strcmp (argv[i], "-s"))
	    (void) strcpy (server, argv[++i]);
        else if (!strcmp (argv[i], "-p"))
            pname = atoi (argv[++i]);
	else if (!strcmp (argv[i], "-d"))
	    debug = TRUE;
	else {
	    (void) fprintf (stderr, "Usage: CmdTool [-d] [-h hostname] "
			    "[-s server] [-p port]\n");
	    exit(-1);
	}
    }

    /* establish connection to sequencer */
    if ((msgfd = net_connect (server, hostname, pname, BLOCKING)) < 0 ) {
        (void)fprintf(stderr,"CmdTool: Could not connect to server: error: %s\n", 
			NET_ERRSTR(msgfd));
	exit(msgfd);
    }

    main_loop();

    (void) net_close(msgfd);

    exit(0);
}



void main_loop () {
    RspMsg  *rsp_msg;
    fd_set  readfds;
    char    cmd_line[256], *error;
    int	    i, n, status, prompt = 1;	

    rsp_msg = (RspMsg*)malloc(NET_MAX_MSG_LEN);
    for (;;) {

	if (prompt) fprintf (stderr, "%s > ", hostname);

        FD_ZERO (&readfds);

	if (! quitting)
            FD_SET (0, &readfds);

        FD_SET (msgfd, &readfds);

	/* wait for input from one of file descriptors */
        n = select (FD_SETSIZE, &readfds, (fd_set *) 0, (fd_set *) 0,
                      (struct timeval *) 0);
        if (n <= 0) {
            fprintf (stderr, "Select() fails, %s\n", strerror(errno));
	    break;
        }

	/* check if data received from sequencer */
	if (FD_ISSET (msgfd, &readfds)) {
	    
            n = net_recv (msgfd,(char *)rsp_msg, NET_MAX_MSG_LEN, BLOCKING);

	    if (n <= 0) {
		fprintf (stderr, "Connection broken... %s\n",
			 n ==  0 ? "" :
			 n == -1 ? strerror(errno) : NET_ERRSTR(n));
		break;
            }
	    if (ntohl(rsp_msg->hdr.msgId) != RSP_TYPE) {

	      fprintf (stderr, "Message ID 0x%X received - ignored.\n", 
		       ntohl(rsp_msg->hdr.msgId));
	    }
	    else {
		fprintf (stderr, "%s\n", rsp_msg->rsp);
	    }
	    if (quitting)
		break;
	}

	/* check if user has entered input from command line */
	if (FD_ISSET (0, &readfds)) {

	    if (fgets (cmd_line,sizeof(cmd_line), stdin) == NULL || !strcasecmp(cmd_line, "quit")) {
		quitting = 1;
		continue;
	    }
	    if (strncmp("!!",cmd_line, 2) == 0) {
		strcpy(cmd_line, last_cmd);
	    }
	    else {
		strcpy(last_cmd, cmd_line);
	    }

	    for (i=0; isspace(cmd_line[i]); ++i);

	    if (cmd_line[i] == 0)
		continue;

	    status = send_cmd (msgfd, cmd_line, &error );
	    
	    if (status < 0) {
	        fprintf (stderr,"%s\n", error);  
		continue;
	    }
	}
    }

}

int send_cmd ( int fd, char *cmdStr, char **error ) {
    int       stat;

    *error = NULL;

    if ((fd == -1) || (cmdStr == NULL)) {
        *error = Invalid_Parameters;
        return ERROR;
    }

    msg.hdr.msgId = htonl (CMD_TYPE);
    msg.hdr.srcId = htonl (ANY_TASK);
    msg.hdr.msgLen = htonl (sizeof(CmdMsg));

    strncpy (msg.cmd, cmdStr, MAX_CMD_LEN);

    if ((stat = net_send(fd, (char*)&msg, sizeof (msg), BLOCKING)) < 0) {
        *error = Error_Sending;
        return stat;
    }
    else {
        return (SUCCESS);
    }
}
