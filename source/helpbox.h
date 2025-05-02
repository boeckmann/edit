/* --------- helpbox.h ----------- */

#ifndef HELPBOX_H
#define HELPBOX_H

#include <stdint.h>
/* --------- linked list of help text collections -------- */
struct helps {
    char *hname;
	char *comment;
    uint32_t hptr;
    uint16_t bit;
    uint16_t hheight;
    uint16_t hwidth;
	uint16_t nexthlp;
	uint16_t prevhlp;
    void *hwnd;
	char *PrevName;
	char *NextName;
#ifdef FIXHELP
	struct helps *NextHelp;
#endif
};

#endif

