/* --------- helpbox.h ----------- */

#ifndef HELPBOX_H
#define HELPBOX_H

/* --------- linked list of help text collections -------- */
struct helps {
    char *hname;
	char *comment;
    u32 hptr;
    u16 bit;
    u16 hheight;
    u16 hwidth;
	u16 nexthlp;
	u16 prevhlp;
    void *hwnd;
	char *PrevName;
	char *NextName;
#ifdef FIXHELP
	struct helps *NextHelp;
#endif
};

#endif

