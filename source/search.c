/* ---------------- search.c ------------- */
#include "dfpcomp.h"

extern DBOX SearchTextDB;
extern DBOX ReplaceTextDB;
static int CheckCase = FALSE;
static int Replacing = FALSE;
static int lastsize;

/* - case-insensitive, white-space-normalized char compare - */
static BOOL SearchCmp(int a, int b)
{
    if (b == '\n')
        b = ' ';
    if (CheckCase)
        return a != b;
    return tolower(a) != tolower(b);
}

/* ----- replace a matching block of text ----- */
static void replacetext(WINDOW wnd, char *cp1, DBOX *db)
{
    char *cr = GetEditBoxText(db, ID_REPLACEWITH);
    char *cp = GetEditBoxText(db, ID_SEARCHFOR);
    int oldlen = strlen(cp); /* length of text being replaced */
    int newlen = strlen(cr); /* length of replacing text      */
    int dif;
	lastsize = newlen;
    if (oldlen < newlen)    {
        /* ---- new text expands text size ---- */
        dif = newlen-oldlen;
        if (wnd->textlen < strlen(wnd->text)+dif)    {
            /* ---- need to reallocate the text buffer ---- */
            int offset = (int)(cp1-wnd->text);
            wnd->textlen += dif;
            wnd->text = DFrealloc(wnd->text, wnd->textlen+2);
            cp1 = wnd->text + offset;
        }
        memmove(cp1+dif, cp1, strlen(cp1)+1);
    }
    else if (oldlen > newlen)    {
        /* ---- new text collapses text size ---- */
        dif = oldlen-newlen;
        memmove(cp1, cp1+dif, strlen(cp1)+1);
    }
    strncpy(cp1, cr, newlen);
}

/* ------- search for the occurrance of a string ------- */
static void SearchTextBox(WINDOW wnd, int incr)
{
    char *s1 = NULL, *s2, *cp1;
    DBOX *db = Replacing ? &ReplaceTextDB : &SearchTextDB;
    char *cp = GetEditBoxText(db, ID_SEARCHFOR);
    BOOL rpl = TRUE;
    static FoundOne;
    static int fromBeginning;
    static int OrigCol, OrigLine;

    if (!incr) {
        FoundOne = FALSE;
        OrigCol = wnd->CurrCol;
        OrigLine = wnd->CurrLine;
        fromBeginning = wnd->CurrCol == 0 && wnd->CurrLine == 0;
    }

    if (Replacing && CheckBoxSetting(&ReplaceTextDB, ID_REPLACEALL)) {
        wnd->CurrCol = 0;
        wnd->CurrLine = 0;
        fromBeginning = 1;        
    }

again:
    while (rpl == TRUE && cp != NULL && *cp)    {
        /* even for "find again" (not replacing) this runs at least once */
        rpl = Replacing ?
                CheckBoxSetting(&ReplaceTextDB, ID_REPLACEALL) :
                FALSE;
        if (TextBlockMarked(wnd))    {
            ClearTextBlock(wnd); /* un-mark block */
            SendMessage(wnd, PAINT, 0, 0);
        }
        /* search for a match starting at cursor position */
        cp1 = CurrChar;
        if (incr)
            cp1 += lastsize;    /* start past the last hit */
        /* --- compare at each character position --- */
        while (*cp1)    {
            s1 = cp;
            s2 = cp1;
            while (*s1 && *s1 != '\n')    {
                if (SearchCmp(*s1, *s2))
                    break;
                s1++, s2++;
            }
            if (*s1 == '\0' || *s1 == '\n')
                break;
            cp1++;
        }
        if (s1 != NULL && (*s1 == 0 || *s1 == '\n'))    {
            /* ----- match at *cp1 ------- */
            FoundOne = TRUE;

            /* mark a block at beginning of matching text */
            wnd->BlkEndLine = TextLineNumber(wnd, s2);
            wnd->BlkBegLine = TextLineNumber(wnd, cp1);
            if (wnd->BlkEndLine < wnd->BlkBegLine)
                wnd->BlkEndLine = wnd->BlkBegLine;
            wnd->BlkEndCol =
                (int)(s2 - TextLine(wnd, wnd->BlkEndLine));
            wnd->BlkBegCol =
                (int)(cp1 - TextLine(wnd, wnd->BlkBegLine));

            /* position the cursor at the matching text */
            wnd->CurrCol = wnd->BlkBegCol;
            wnd->CurrLine = wnd->BlkBegLine;

			/* -- remember the size of the matching text -- */
			lastsize = strlen(cp);

            /* align the window scroll to matching text */
            if (lastsize < ClientWidth(wnd)-1) {
                if (WndCol + lastsize > ClientWidth(wnd)-1) {
                    wnd->wleft = wnd->CurrCol - (ClientWidth(wnd)-lastsize);
                }
            }
            else if (WndCol > ClientWidth(wnd)-1) {
                wnd->wleft = wnd->CurrCol;
            }
            if (wnd->CurrCol < wnd->wleft) {
                wnd->wleft = wnd->CurrCol;
            }

            wnd->wtop = min(max(wnd->CurrLine - 7, 0), wnd->wlines - ClientHeight(wnd));
            wnd->WndRow = wnd->CurrLine - wnd->wtop;

            SendMessage(wnd, PAINT, 0, 0);
            SendMessage(wnd, KEYBOARD_CURSOR,
                WndCol, wnd->WndRow);

            if (Replacing)    {
                if (rpl || YesNoBox("Replace the text?"))  {
                    replacetext(wnd, cp1, db);
                    wnd->TextChanged = TRUE;
                    BuildTextPointers(wnd);
                	if (rpl)    {
                    	incr = TRUE;
                    	continue;
                	}
                }
                ClearTextBlock(wnd);
                SendMessage(wnd, PAINT, 0, 0);
            }
            return;
        }
        break;
    }
    if (!fromBeginning) {
        if (YesNoBox("End reached. Start from beginning?")) {
            wnd->CurrCol = 0;
            wnd->CurrLine = 0;
            fromBeginning = 1;
            rpl = TRUE;
            incr = FALSE;
            goto again;             
        }
    } else {
        if (FoundOne && !(Replacing && CheckBoxSetting(&ReplaceTextDB, ID_REPLACEALL)))
            MessageBox("Search", "No more occurances.");
    }

    if (!FoundOne) {
        MessageBox("Search", "No matching text found");
        wnd->CurrCol = OrigCol;
        wnd->CurrLine = OrigLine;
    }
}

/* ------- search for the occurrance of a string,
        replace it with a specified string ------- */
void ReplaceText(WINDOW wnd)
{
	Replacing = TRUE;
	lastsize = 0;
    if (CheckCase)
        SetCheckBox(&ReplaceTextDB, ID_MATCHCASE);
    if (DialogBox(NULL, &ReplaceTextDB, TRUE, NULL))    {
        CheckCase=CheckBoxSetting(&ReplaceTextDB,ID_MATCHCASE);
        SearchTextBox(wnd, FALSE);
    }
}

/* ------- search for the first occurrance of a string ------ */
void SearchText(WINDOW wnd)
{
	Replacing = FALSE;
	lastsize = 0;
    if (CheckCase)
        SetCheckBox(&SearchTextDB, ID_MATCHCASE);
    if (DialogBox(NULL, &SearchTextDB, TRUE, NULL))    {
        CheckCase=CheckBoxSetting(&SearchTextDB,ID_MATCHCASE);
        SearchTextBox(wnd, FALSE);
    }
}

/* ------- search for the next occurrance of a string ------- */
void SearchNext(WINDOW wnd)
{
    SearchTextBox(wnd, TRUE);
}

