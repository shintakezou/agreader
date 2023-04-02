/*** Basic functions to do with TTY ***/

#ifndef IO_TTY_H
#define IO_TTY_H

/** Switch between normal (0) or raw (1) mode **/
void raw_mode(int);

/** Test whether raw mode is on **/
char is_rawmode(void);

/** Open raw modes for getting character from keyboard **/
void open_getchr(void);

/** Get a character from standard input (keyboard) **/
char getchr(void);

/** Get terminal dimension **/
void get_termsize(short*);

/** Set the region affected by the scrolling **/
void set_scroll_region(short height);

/** Set cursor to screen postion **/
void set_cursor_pos(short y, short x);

/** Init (1) or remove (0) signal handler SIGINT and SIGWINCH **/
typedef void (*sighandler_t)(int);
void init_signals(int on, sighandler_t sig_int, sighandler_t sig_winch);

/** Simple integer to string conversion **/
char* my_itoa(char*, short);

#endif
