/*	$Id: mailload.c,v 1.2 2009/04/14 15:21:09 liusf Exp $	*/

/*
 * Copyright (c) 2003, 2004, 2005, 2006 Marc Balmer <marc@msys.ch>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT 60
#define BUFLEN	255

#define strlcpy strncpy
#define strlcat strncat

time_t start_time;
time_t end_time;
time_t stop_time;

int verbose;

char *host;
int port;

char *from;
char *to;
char *subject;

int msgs;	/* total number of messages sent */
double mpm, mps, spm;

int nsenders;	/* number of parallel senders */
int nmsgs;	/* number of messages per connection */
int nbytes;	/* number of bytes per message */

int nmessages;	/* number of messages to send */
int nseconds;	/* number of seconds to run */

int nterminat;	/* approx. number of terminated child processes */
int totsenders;	/* total number of senders used */

volatile sig_atomic_t time_to_quit;

extern int copy(FILE *, FILE *);

struct senderinf {
	pthread_t	tid;
	int		senderid;
	int		conns;
	int		msgs;
};

void
usage(void)
{
	fputs("usage: smtpsend [options] [host]\n"
	    "options:\n"
	    "\t-s senders\tNumber of parallel senders\n"
	    "\t-m messages\tSend n messages per connection\n"
	    "\t-b bytes\tMessage size in bytes\n"
	    "\noperation modes (exactly one is required):\n"
	    "\t-n messages\tSend at least n messages\n"
	    "\t-t seconds\tRun for n seconds\n"
	    "\nmiscellaneous:\n"
	    "\t-p port\t\tPort number to connect to\n"
	    "\t-F from_address\tSpecify the senders e-mail address\n"
	    "\t-T to_address\tSpecify the recipients e-mail address\n"
	    "\t-S subject\tSpecify subject of the message\n"
	    "\t-v \t\tBe verbose (give twice to show SMTP traffic)\n", stderr);
	exit(1);
}

void
send_line(FILE *fp, char *line)
{
	fprintf(fp,  "%s\r\n", line);

	if (verbose > 1)
		printf("-> %s\n", line);
}

char
get_response(FILE *fp)
{
	char buf[BUFLEN];

	buf[0] = '?';

	if (!fgets(buf, sizeof(buf), fp))
		warnx("no response from server");
	else if (verbose > 1)
		printf("<- %s", buf);

	return buf[0];
}

char
smtp_send(FILE *fp, char *s)
{
	send_line(fp, s);
	return get_response(fp);
}

int
smtp_message(FILE *fp)
{
	char buf[BUFLEN];
	int verbosity;
	int size;

	strlcpy(buf, "MAIL FROM:<", sizeof(buf));
	strlcat(buf, from, sizeof(buf));
	strlcat(buf, ">", sizeof(buf));

	if (smtp_send(fp, buf) != '2') {
		warnx("protocol error, MAIL FROM");
		return -1;
	}

	strlcpy(buf, "RCPT TO:<", sizeof(buf));
	strlcat(buf, to, sizeof(buf));
	strlcat(buf, ">", sizeof(buf));

	if (smtp_send(fp, buf) != '2') {
		warnx("protocol error, RCPT TO");
		return -1;
	}

	strlcpy(buf, "DATA", sizeof(buf));

	if (smtp_send(fp, buf) != '3') {
		warnx("protocol error, DATA");
		return -1;
	}

	verbosity = verbose;

	strlcpy(buf, "To: ", sizeof(buf));
	strlcat(buf, to, sizeof(buf));
	send_line(fp, buf);

	strlcpy(buf, "From: ", sizeof(buf));
	strlcat(buf, from, sizeof(buf));
	send_line(fp, buf);

	strlcpy(buf, "Subject: ", sizeof(buf));
	strlcat(buf, subject, sizeof(buf));
	send_line(fp, buf);
	send_line(fp, "X-Mailer: smtpsend\r\n");

	/*
	 * Change the following message generator to something more
	 * intelligent some day...
	 */
	strlcpy(buf, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890", sizeof(buf));

	for (size = nbytes; size > 0; size -= 36)
		send_line(fp, buf);

	if (smtp_send(fp, ".") != '2') {
		warnx("protocol error, after DATA");
		return -1;
	}

	return 0;
}

int
smtp_connection(void)
{
	int fd;
	FILE *fp;
	struct sockaddr_in server_sockaddr;
	struct hostent *hostent;
	int msgs;

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		warnx("unable to obtain network");
		return 0;
	}
	bzero((char *) &server_sockaddr, sizeof(server_sockaddr));

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_port = htons(port);

	if (inet_aton(host, &server_sockaddr.sin_addr) == 0) {
		hostent = gethostbyname(host);
		if (!hostent) {
			warnx("unknown host: %s", host);
			return 0;
		}

		server_sockaddr.sin_family = hostent->h_addrtype;
		memcpy(&server_sockaddr.sin_addr, hostent->h_addr,
		    hostent->h_length);
	}

	if (connect(fd, (struct sockaddr *)&server_sockaddr,
	    sizeof(server_sockaddr)) == -1) {
		warnx("unable to connect socket, errno=%d, %s", errno,
		    strerror(errno));
		return 0;
	}

	if (!(fp = fdopen(fd, "a+"))) {
		warnx("can't open output");
		fclose( fp );
		return 0;
	}

	if (get_response(fp) != '2') {
		warnx("error reading connection");
		fclose( fp );
		return 0;
	}

	if (smtp_send(fp, "HELO localhost") != '2') {
		warnx("protocol error in connection setup");
		fclose( fp );
		return 0;
	}

	for (msgs = 0; !time_to_quit && msgs < nmsgs; msgs++) {
		if (smtp_message(fp)) {
			warnx("error sending message");
			fclose( fp );
			return 0;
		}

		if (smtp_send(fp, "RSET") != '2') {
			warnx("protocol error, RSET");
			fclose( fp );
			return 0;
		}
	}

	smtp_send(fp, "QUIT");
	fclose(fp);

	return msgs;
}

void
sigalrm(int signo)
{
	time_to_quit = 1;
}

void *
timer(void *arg)
{
	time_t now;

	do {
		time(&now);
		if (now > start_time + nseconds) {
			if (verbose)
				puts("benchmark ends, waiting for senders "
				    "to terminate");
			time_to_quit = 1;
		}
	} while (!time_to_quit);

	return NULL;
}


void *
sender(void *arg)
{
	struct senderinf *inf;

	inf = (struct senderinf *) arg;
	inf->conns = inf->msgs = 0;

	while (!time_to_quit) {
		inf->msgs += smtp_connection();
		++inf->conns;
	}
	return NULL;
}

int
main(int argc, char *argv[])
{
	int ch;
	long time_elapsed;
	pthread_t tid;
	struct senderinf *senderinf;
	char *ep;
	int i;

	host = "127.0.0.1";
	port = 25;
	from = "smtpsend@localhost";
	to = "smtpsink@localhost";
	subject = "smtpsend";
	nsenders = 1;
	nmsgs = 1;
	nbytes = 1024;

	while ((ch = getopt(argc, argv, "b:F:m:n:p:S:s:T:t:v?")) != -1) {
		switch (ch) {
		case 'b':
			nbytes = (int)strtol(optarg, &ep, 10);
			if (nbytes <= 0 || *ep != '\0') {
				warnx("illegal number, -b argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'F':
			from = optarg;
			break;
		case 'm':
			nmsgs = (int)strtol(optarg, &ep, 10);
			if (nmsgs <= 0 || *ep != '\0') {
				warnx("illegal number, -m argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'n':
			nmessages = (int)strtol(optarg, &ep, 10);
			if (nmessages <= 0 || *ep != '\0') {
				warnx("illegal number, -n argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'p':
			port = (int)strtol(optarg, &ep, 10);
			if (port <= 0 || *ep != '\0') {
				warnx("illegal number, -p argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'S':
			subject = optarg;
			break;
		case 's':
			nsenders = (int)strtol(optarg, &ep, 10);
			if (nsenders <= 0 || *ep != '\0') {
				warnx("illegal number, -s argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'T':
			to = optarg;
			break;
		case 't':
			nseconds = (int)strtol(optarg, &ep, 10);
			if (nseconds <= 0 || *ep != '\0') {
				warnx("illegal number, -t argument -- %s",
				    optarg);
				usage();
			}
			break;
		case 'v':
			++verbose;
			break;
		default:
			usage();
		}
	}

	if ((nmessages == 0 && nseconds == 0)
	    || (nmessages != 0 && nseconds != 0))
		usage();

	if (nsenders > 1 && verbose > 1) {
		puts("limiting number of senders to one");
		nsenders = 1;
	}

	argc -= optind;
	argv += optind;

	if (argc == 1)
		host = argv[0];

	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, sigalrm);

	if ((senderinf = (struct senderinf*)calloc(nsenders, sizeof(struct senderinf))) == NULL)
		err(1, "memory allocation error");

	if (!nseconds)
		exit(0);

	if (verbose)
		puts("starting benchmark");

	time(&start_time);

	for (i = 0; i < nsenders; i++) {
		senderinf[i].senderid = i;
		if (pthread_create(&senderinf[i].tid, NULL, sender,
		    &senderinf[i]))
			err(1, "thread creation failed");
	}

	if (pthread_create(&tid, NULL, timer, NULL))
		err(1, "timer thread creation failed");

	for (i = 0; i < nsenders; i++)
		pthread_join(senderinf[i].tid, NULL);	
	pthread_join(tid, NULL);

	time(&end_time);

	if (verbose) {
		puts("benchmark ended, per sender stats are as follows:\n");
		puts(" sender | connections | messages"); 
		puts("--------|-------------|----------");
	}

	for (i = 0; i < nsenders; i++) {
		if (verbose)
			printf(" %5d  | %10d  | %7d\n", i, senderinf[i].conns,
			    senderinf[i].msgs); 
		msgs += senderinf[i].msgs;
	}

	if (verbose)
		puts("");	/* newline */

	free(senderinf);

	time_elapsed = (long)end_time - start_time;
	mps = (double)msgs / (double)time_elapsed;
	spm = (double)time_elapsed / (double)msgs;
	mpm = (double)msgs / ((double)time_elapsed / 60.0);
	printf("Sent %d messages in %ld seconds\n", msgs, time_elapsed);
	printf("Sending rate: %8.2f messages/minute, %8.2f messages/second\n",
	    mpm, mps);
	printf("Average delivery time: %8.2f seconds/message\n", spm);

	return 0;
}
