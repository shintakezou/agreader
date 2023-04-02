/****************************************************************
***** io_tty.c: Various terminal related specific procedures ****
*****           Taken from less source code by Mark Nudelman ****
*****           but greatly simplified by T.Pierron          ****
****************************************************************/

#include <strings.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>

#include "AGReader.h"
#include "IO_tty.h"
#include <signal.h>

int tty; /* tty used to display all things */
int is_tty; /* 1 if file is being read from a pipe */
char underlined = 1; /* 1 if terminal support underlined mode */

/** This sequence is to get a private mode in a xterm & co ***
*** while viewing a file (quoted from konsole tech specs)  **/
char SET_PRIV[] = "\0337" /* Save cursor position */
                  "\033[?47;1h"; /* Enter in private mode (47) and set mode AppCuKeys (1) */

/** This sequence is to get back in normal mode **/
char SET_PUB[] = "\033[2J" /* Clears screen */
                 "\033[?1;47l" /* Reset AppCuKeys (1) and private (47) mode */
                 "\0338"; /* Restore cursor position */

/** Current state mode (0:normal, 1:private) **/
static int old_st = 0;

/* Change terminal to "raw mode", or restore to "normal" mode. This means:
 *	1. An outstanding read will complete on receipt of a single keystroke.
 *	2. Input is not echoed.
 *	3. On output, \n is mapped to \r\n.
 *	4. \t is NOT expanded into spaces.
 *	5. Signal-causing characters such as ctrl-C (interrupt),
 *	   etc. are NOT disabled.
 * It doesn't matter whether an input \n is mapped to \r, or vice versa.
 */
void raw_mode(int on)
{
    static struct termios save_term;
    struct termios s;
    int i; /* silence warnings */

    /* Do not set twice the same mode!! */
    if (old_st == on)
        return;
    old_st = on;
    if (on) {
        /* Get terminal modes */
        tcgetattr(2, &s);

        /* Save modes and set certain variables dependent on modes */
        save_term = s;

        /* Set the modes to the way we want them */
        s.c_lflag &= ~(0
#ifdef ICANON
            | ICANON
#endif
#ifdef ECHO
            | ECHO
#endif
#ifdef ECHOE
            | ECHOE
#endif
#ifdef ECHOK
            | ECHOK
#endif
#if ECHONL
            | ECHONL
#endif
        );

        s.c_oflag |= (0
#ifdef XTABS
            | XTABS
#else
#ifdef TAB3
            | TAB3
#else
#ifdef OXTABS
            | OXTABS
#endif
#endif
#endif
#ifdef OPOST
            | OPOST
#endif
#ifdef ONLCR
            | ONLCR
#endif
        );

        s.c_oflag &= ~(0
#ifdef ONOEOT
            | ONOEOT
#endif
#ifdef OCRNL
            | OCRNL
#endif
#ifdef ONOCR
            | ONOCR
#endif
#ifdef ONLRET
            | ONLRET
#endif
        );

        /* Get some feature of host terminal */
        {
            char* env;
            if ((env = getenv("TERM")) != NULL && !strcasecmp(env, "linux"))
                /* Unfortunately linux console disabled all previous defined **
                ** styles in an ANSI sequence that contains underlining mode */
                underlined = 0;
        }

        s.c_cc[VMIN] = 1;
        s.c_cc[VTIME] = 0;

        /* let's enter in private mode */
        i = write(1, SET_PRIV, sizeof(SET_PRIV) - 1);
    } else {
        /* Restore saved modes */
        s = save_term;
        /* and old display mode */
        i = write(1, SET_PUB, sizeof(SET_PUB) - 1);
    }
    tcsetattr(2, TCSADRAIN, &s);
}

/*** Test whether raw mode is on ***/
char is_rawmode(void)
{
    return (old_st == 1);
}

void open_getchr(void)
{
    /* Try /dev/tty. If that doesn't work, use file descriptor 2 **
    ** which in Unix is usually attached to the screen, but also **
    ** usually lets you read from the keyboard                   */
    tty = open("/dev/tty", O_RDONLY);
    if (tty < 0)
        tty = 2;
}

#if 0
/*** Get a character from the keyboard ***/
char getchr(void)
{
	char c;
	/* If file is read from a pipe, file-descriptor **
	** must be surveyed as well as standard input   */
	if( is_tty == 0 )
	{
		static fd_set fds;
		static struct timeval tv;
		char refresh;
		for(refresh=0; ; ) {
			FD_ZERO(&fds);
			FD_SET(0,   &fds);
			FD_SET(tty, &fds);
			tv.tv_sec  = 0;
			tv.tv_usec = 100000;
			if( select(tty+1, &fds, NULL, NULL, refresh ? &tv : NULL) )
			{
				if( FD_ISSET(0, &fds) )
					if( UpdateBuffer() == 0) is_tty=0, ThrowError("End of buffer",NULL);
					else refresh=1;
				if( FD_ISSET(tty, &fds) ) {
					read(tty, &c, sizeof(c));
					return c;
				}
			} else RefreshBuffer(), refresh=0; /* timeout */
		}
	} else
		/* Read just a single character from stdin */
		read(tty, &c, sizeof(c));

	return c;
}
#else
char getchr(void)
{
    char c;
    if (read(tty, &c, sizeof(c)) != sizeof(c))
        return 0;

    return c;
}
#endif

/*** Set the scrolling region ***/
void set_scroll_region(short height)
{
    /* This sequence sets the vertical scrolling region */
    printf("\033[1;%dr", height);
    fflush(stdout);
}

/*** Get dimension of the current terminal ***/
void get_termsize(short* wh)
{
    struct winsize w;
    char* s;
    wh[0] = 0;
    wh[1] = 0;

    /* Send a command to the terminal, to get its size */
    if (ioctl(1, TIOCGWINSZ, &w) == 0) {
        if (w.ws_col > 0)
            wh[0] = w.ws_col;
        if (w.ws_row > 0)
            wh[1] = w.ws_row;
    }

    /* Check if sizes are right, else try to get environment variables */
    if (wh[0] <= 0 && (s = getenv("COLUMNS")) != NULL)
        wh[0] = atoi(s);
    if (wh[1] <= 0 && (s = getenv("LINES")) != NULL)
        wh[1] = atoi(s);

    /* If this fails too, use fixed dimension */
    if (wh[0] <= 0)
        wh[0] = 80;
    if (wh[1] <= 0)
        wh[1] = 24;
}

/*** Set the cursor to the specified screen position ***/
void set_cursor_pos(short line, short col)
{
    /* The special escape sequence is: \e[Py;PxH */
    printf("\033[%d;%dH", line, col);
}

/*** Setup various handler ***/
void init_signals(int on, sighandler_t sig_int, sighandler_t sig_winch)
{
    if (on) {
        /* Set signal handlers */
        signal(SIGINT, sig_int);
        signal(SIGWINCH, sig_winch);
    } else {
        /* Reset original signal handlers */
        signal(SIGINT, SIG_DFL);
        signal(SIGWINCH, SIG_DFL);
    }
}
