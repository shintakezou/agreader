/**********************************************************
** Help.h : String displayed as command reference list   **
**          by T.Pierron, 9 oct 2000, free software.     **
**********************************************************/

#ifndef	NO_STR_HELP
#include "Version.h"
#include "Text.h"

#define	LGR(str)				"\033[32;1m" str "\033[0m"
#define	DGR(str)				"\033[32m" str "\033[0m"
#define	LST(str1,str2)		LGR(str1) ", " str2

#define	STR_HELP		\
"\n\t\t\033[1mAGReader v" SVER " Command Reference\033[0m\n\n" \
"  This is a very quick reference, \033[4mread\033[0m documentation for full detail.\n\n\033[1m" \
"Shortcut....... What it does ____________________________________\033[0m\n" \
 LST("q", LGR("CTRL/C")) "...... Quit the program.\n" \
 DGR("cursor") "......... Move up/down one line, 5 columns left/right.\n" \
 LST("i",LST("j",LST("k",LGR("l")))) "..... Cursor equivalent (if latter doesn't work).\n" \
 LST("PgUp", LGR("PgDown")) "... Up/down one page (window height dependant).\n" \
 LST("I", LGR("K")) "........... Replacement for PgUp/PgDown.\n" \
 LST("Home", LGR("End")) "...... Go to beginning/end of the displayed node.\n" \
 LST("g", LGR("G")) "........... Equivalent to Home/End.\n" \
 LST("TAB", LGR("a | p")) "..... Find the next/previous AG link.\n" \
 LGR("F1") "............. Display current Help node, if any.\n" \
 LGR("F2") "............. Display Index of current node, if any.\n" \
 LGR("F3") "............. Display Table Of Content of node, if any.\n" \
 LST("Space", LGR("Enter")) "... Navigate through the activated AG link.\n" \
 LST("n", LGR("b")) "........... Search for next/previous node defined by the AG file.\n" \
 LGR("Backspace") "...... Return back to the previous visited node.\n" \
 LST("+", LGR("-")) "........... Increase/decrease current tabstop.\n" \
 LGR("t") ".............. Show current tabstop value.\n" \
 LGR("CTRL/L") "......... Refreshes the whole screen.\n" \
 LGR("?") ".............. Display information about file viewed.\n" \
 LGR("v") ".............. Show content of the activated link (ie:\"system\" link).\n" \
 LGR("=") ".............. Display current line number.\n" \
 LGR("C") ".............. Convert Amiga colors to Unix ones for ANSI text file.\n" \
"\n\033[1mPress BS to quit this page\033[0m\n\n"

#endif
