/* ------------- editbox.c ------------ */
#include "dfpcomp.h"


#define EditBufLen(wnd) (isMultiLine(wnd) ? EDITLEN : ENTRYLEN)
#define SetLinePointer(wnd, ln) (wnd->CurrLine = ln)

#define FANCY_CTRL_P	/* kludgy eye-candy: ^P clock / statusbar handling */

#ifdef TAB_TOGGLING
#define Ch(c) ((c)&0x7f)	/* ignore high bit for whitespace check  */
				/* (for tab-substitute and tabby-spaces) */
#define isWhite(c) ( (Ch(c)==' ') || (Ch(c)=='\n') || (Ch(c)=='\f') || (Ch(c)=='\t') )
#else
#define isWhite(c) ( ((c)==' ') || ((c)=='\n') || ((c)=='\t') )
#endif

/* ---------- local prototypes ----------- */
static void SaveDeletedText(WINDOW, char *, unsigned int); /* *** UNSIGNED *** */
static void Forward(WINDOW);
static void Backward(WINDOW);
static void End(WINDOW);
static void Home(WINDOW);
static void Downward(WINDOW);
static void Upward(WINDOW);
static void StickEnd(WINDOW);
static void NextWord(WINDOW);
static void PrevWord(WINDOW);
static void ModTextPointers(WINDOW, int, int);
static void SetAnchor(WINDOW, int, int);
/* -------- local variables -------- */
static BOOL KeyBoardMarking, ButtonDown;
static BOOL TextMarking;
static int ButtonX, ButtonY;
static int PrevY = -1;

/* ----------- CREATE_WINDOW Message ---------- */
static int CreateWindowMsg(WINDOW wnd)
{
    int rtn = BaseWndProc(EDITBOX, wnd, CREATE_WINDOW, 0, 0);
    /* *** added in 0.6e *** */
    wnd->BlkBegLine = 0;
    wnd->BlkBegCol = 0;
    wnd->BlkEndLine = 0;
    wnd->BlkEndCol = 0;
    wnd->TextChanged = FALSE;
    wnd->DeletedText = NULL;
    wnd->DeletedLength = 0;
    /* *** /added *** */
    wnd->MaxTextLength = MAXTEXTLEN+1;
    wnd->textlen = EditBufLen(wnd);
    wnd->InsertMode = TRUE;
	if (isMultiLine(wnd))
	    wnd->WordWrapMode = TRUE;
	SendMessage(wnd, CLEARTEXT, 0, 0);
    return rtn;
}
/* ----------- SETTEXT Message ---------- */
static int SetTextMsg(WINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (strlen((char *)p1) <= wnd->MaxTextLength)	{
        rtn = BaseWndProc(EDITBOX, wnd, SETTEXT, p1, 0);
	    wnd->TextChanged = FALSE;
	}
    return rtn;
}
/* ----------- CLEARTEXT Message ------------ */
static int ClearTextMsg(WINDOW wnd)
{
    int rtn = BaseWndProc(EDITBOX, wnd, CLEARTEXT, 0, 0);
    unsigned blen = EditBufLen(wnd)+2;
    wnd->text = DFrealloc(wnd->text, blen);
    memset(wnd->text, 0, blen);
    wnd->wlines = 0;
    wnd->CurrLine = 0;
    wnd->CurrCol = 0;
    wnd->WndRow = 0;
    wnd->wleft = 0;
    wnd->wtop = 0;
    wnd->textwidth = 0;
    wnd->TextChanged = FALSE;
    return rtn;
}
/* ----------- ADDTEXT Message ---------- */
static int AddTextMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn = FALSE;
    if (strlen((char *)p1)+wnd->textlen <= wnd->MaxTextLength) {
        rtn = BaseWndProc(EDITBOX, wnd, ADDTEXT, p1, p2);
        if (rtn != FALSE)    {
            if (!isMultiLine(wnd))    {
                wnd->CurrLine = 0;
                wnd->CurrCol = strlen((char *)p1);
                if (wnd->CurrCol >= ClientWidth(wnd))    {
                    wnd->wleft = wnd->CurrCol-ClientWidth(wnd);
                    wnd->CurrCol -= wnd->wleft;
                }
                wnd->BlkEndCol = wnd->CurrCol;
                SendMessage(wnd, KEYBOARD_CURSOR,
                                     WndCol, wnd->WndRow);
            }
        }
    }
    return rtn;
}
/* ----------- GETTEXT Message ---------- */
static int GetTextMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    char *cp1 = (char *)p1;
    char *cp2 = wnd->text;
    if (cp2 != NULL)    {
        while (p2-- && *cp2 && *cp2 != '\n')
            *cp1++ = *cp2++;
        *cp1 = '\0';
        return TRUE;
    }
    return FALSE;
}
/* ----------- SETTEXTLENGTH Message ---------- */
static int SetTextLengthMsg(WINDOW wnd, unsigned int len)
{
    if (++len < MAXTEXTLEN)    {
        wnd->MaxTextLength = len;
        if (len < wnd->textlen)    {
            wnd->text=DFrealloc(wnd->text, len+2);
            wnd->textlen = len;
            *((wnd->text)+len) = '\0';
            *((wnd->text)+len+1) = '\0';
            BuildTextPointers(wnd);
        }
        return TRUE;
    }
    return FALSE;
}
/* ----------- KEYBOARD_CURSOR Message ---------- */
static void KeyboardCursorMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    wnd->CurrCol = (int)p1 + wnd->wleft;
    wnd->WndRow = (int)p2;
    wnd->CurrLine = (int)p2 + wnd->wtop;
    if (wnd == inFocus)	{
		if (CharInView(wnd, (int)p1, (int)p2))
	        SendMessage(NULL, SHOW_CURSOR,
				(wnd->InsertMode && !TextMarking), 0);
    	else
			SendMessage(NULL, HIDE_CURSOR, 0, 0);
	}
}
/* ----------- SIZE Message ---------- */
int SizeMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn = BaseWndProc(EDITBOX, wnd, SIZE, p1, p2);
    if (WndCol > ClientWidth(wnd)-1)
        wnd->CurrCol = ClientWidth(wnd)-1 + wnd->wleft;
    if (wnd->WndRow > ClientHeight(wnd)-1)    {
        wnd->WndRow = ClientHeight(wnd)-1;
        SetLinePointer(wnd, wnd->WndRow+wnd->wtop);
    }
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return rtn;
}
/* ----------- SCROLL Message ---------- */
static int ScrollMsg(WINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (isMultiLine(wnd))    {
        rtn = BaseWndProc(EDITBOX,wnd,SCROLL,p1,0);
        if (rtn != FALSE)    {
            if (p1)    {
                /* -------- scrolling up --------- */
                if (wnd->WndRow == 0)    {
                    wnd->CurrLine++;
                    StickEnd(wnd);
                }
                else
                    --wnd->WndRow;
            }
            else    {
                /* -------- scrolling down --------- */
                if (wnd->WndRow == ClientHeight(wnd)-1)    {
                    if (wnd->CurrLine > 0)
                        --wnd->CurrLine;
                    StickEnd(wnd);
                }
                else
                    wnd->WndRow++;
            }
            SendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- HORIZSCROLL Message ---------- */
static int HorizScrollMsg(WINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    char *currchar = CurrChar;
    if (!(p1 &&
            wnd->CurrCol == wnd->wleft && *currchar == '\n'))  {
        rtn = BaseWndProc(EDITBOX, wnd, HORIZSCROLL, p1, 0);
        if (rtn != FALSE)    {
            if (wnd->CurrCol < wnd->wleft)
                wnd->CurrCol++;
            else if (WndCol == ClientWidth(wnd))
                --wnd->CurrCol;
            SendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
        }
    }
    return rtn;
}
/* ----------- SCROLLPAGE Message ---------- */
static int ScrollPageMsg(WINDOW wnd, PARAM p1)
{
    int rtn = FALSE;
    if (isMultiLine(wnd))    {
        rtn = BaseWndProc(EDITBOX, wnd, SCROLLPAGE, p1, 0);
        SetLinePointer(wnd, wnd->wtop+wnd->WndRow);
        StickEnd(wnd);
        SendMessage(wnd, KEYBOARD_CURSOR,WndCol, wnd->WndRow);
    }
    return rtn;
}
/* ----------- HORIZSCROLLPAGE Message ---------- */
static int HorizPageMsg(WINDOW wnd, PARAM p1)
{
    int rtn = BaseWndProc(EDITBOX, wnd, HORIZPAGE, p1, 0);
    if ((int) p1 == FALSE)    {
        if (wnd->CurrCol > wnd->wleft+ClientWidth(wnd)-1)
            wnd->CurrCol = wnd->wleft+ClientWidth(wnd)-1;
    }
    else if (wnd->CurrCol < wnd->wleft)
        wnd->CurrCol = wnd->wleft;
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return rtn;
}


int LineLen(const char *line)
{
    char *nl;

    if (line == NULL) return 0;
    if (*line == '\0') return 0;

    nl = strchr(line, '\n');
    if (nl) {
        return nl-line;
    }
    return strlen(line);
}

/* ----- Extend the marked block to the new x,y position ---- */
static void ExtendBlock(WINDOW wnd, int x, int y)
{
    int bbl, bel;
    int ptop = min(wnd->BlkBegLine, wnd->BlkEndLine);
    int pbot = max(wnd->BlkBegLine, wnd->BlkEndLine);
    char *lp = TextLine(wnd, wnd->wtop+y);
    int len = LineLen(lp);
    x = max(0, min(x, len));
	y = max(0, y);
    wnd->BlkEndCol = min(len, x+wnd->wleft);
    wnd->BlkEndLine = y+wnd->wtop;
    bbl = min(wnd->BlkBegLine, wnd->BlkEndLine);
    bel = max(wnd->BlkBegLine, wnd->BlkEndLine);
    /* *** end-before-beginning is allowed WHILE area size is edited *** */
    while (ptop < bbl)    {
        WriteTextLine(wnd, NULL, ptop, FALSE);
        ptop++;
    }
    for (y = bbl; y <= bel; y++)
        WriteTextLine(wnd, NULL, y, FALSE);
    while (pbot > bel)    {
        WriteTextLine(wnd, NULL, pbot, FALSE);
        --pbot;
    }
}


/* ----------- LEFT_BUTTON Message ---------- */
static int LeftButtonMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int MouseX = (int) p1 - GetClientLeft(wnd);
    int MouseY = (int) p2 - GetClientTop(wnd);
    RECT rc = ClientRect(wnd);
    char *lp;
    int len = 0;
    if (KeyBoardMarking)
        return TRUE;
    if (WindowMoving || WindowSizing)
        return FALSE;
    if (TextMarking)    {
        if (!InsideRect(p1, p2, rc))    {
			int x = MouseX, y = MouseY;
			int dir;
			MESSAGE msg = 0;
            if ((int)p2 == GetTop(wnd))
				y++, dir = FALSE, msg = SCROLL;
            else if ((int)p2 == GetBottom(wnd))
				--y, dir = TRUE, msg = SCROLL;
            else if ((int)p1 == GetLeft(wnd))
				--x, dir = FALSE, msg = HORIZSCROLL;
            else if ((int)p1 == GetRight(wnd))
				x++, dir = TRUE, msg = HORIZSCROLL;
			if (msg != 0)	{
                if (SendMessage(wnd, msg, dir, 0))
                    ExtendBlock(wnd, x, y);
	            SendMessage(wnd, PAINT, 0, 0);
			}
        }
        return TRUE;
    }
    if (!InsideRect(p1, p2, rc))
        return FALSE;
    if (TextBlockMarked(wnd))    {
        ClearTextBlock(wnd); /* un-mark block */
        SendMessage(wnd, PAINT, 0, 0);
    }
    if (wnd->wlines)    {
        if (MouseY > wnd->wlines-1)
            return TRUE;
        lp = TextLine(wnd, MouseY+wnd->wtop);
        len = LineLen(lp);
        MouseX = min(MouseX, len);
        if (MouseX < wnd->wleft)    {
            MouseX = 0;
            SendMessage(wnd, KEYBOARD, HOME, 0);
        }
        ButtonDown = TRUE;
        ButtonX = MouseX;
        ButtonY = MouseY;
    }
    else
        MouseX = MouseY = 0;
    wnd->WndRow = MouseY;
    SetLinePointer(wnd, MouseY+wnd->wtop);

    if (isMultiLine(wnd) ||
        (!TextBlockMarked(wnd)
            && MouseX+wnd->wleft <= len))
        wnd->CurrCol = MouseX+wnd->wleft;
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return TRUE;
}
/* ----------- MOUSE_MOVED Message ---------- */
static int MouseMovedMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int MouseX = (int) p1 - GetClientLeft(wnd);
    int MouseY = (int) p2 - GetClientTop(wnd);
    RECT rc = ClientRect(wnd);
    if (!InsideRect(p1, p2, rc))
        return FALSE;
    if (MouseY > wnd->wlines-1)
        return FALSE;
    if (ButtonDown)    {
        SetAnchor(wnd, ButtonX+wnd->wleft, ButtonY+wnd->wtop);
        TextMarking = TRUE;
		rc = WindowRect(wnd);
        SendMessage(NULL,MOUSE_TRAVEL,(PARAM) &rc, 0);
        ButtonDown = FALSE;
    }
    if (TextMarking && !(WindowMoving || WindowSizing))    {
        ExtendBlock(wnd, MouseX, MouseY);
        return TRUE;
    }
    return FALSE;
}

/* End an "editing the marked area size and position" session AND */
/* ensure that the Beg(inning) is before the End! Swap if needed. */
static void StopMarking(WINDOW wnd)
{
    TextMarking = FALSE;
    if ( wnd->BlkBegLine > wnd->BlkEndLine ) {
        swap(wnd->BlkBegLine, wnd->BlkEndLine);
        swap(wnd->BlkBegCol, wnd->BlkEndCol);
    }
    if ( (wnd->BlkBegLine == wnd->BlkEndLine) &&
         (wnd->BlkBegCol > wnd->BlkEndCol) )
        swap(wnd->BlkBegCol, wnd->BlkEndCol);
}
/* ----------- BUTTON_RELEASED Message ---------- */
static int ButtonReleasedMsg(WINDOW wnd)
{
    ButtonDown = FALSE;
    if (TextMarking && !(WindowMoving || WindowSizing))  {
        /* release the mouse ouside the edit box */
        SendMessage(NULL, MOUSE_TRAVEL, 0, 0);
        StopMarking(wnd);
        return TRUE;
    }
    PrevY = -1;
    return FALSE;
}
/* ---- Process text block keys for multiline text box ---- */
static void DoMultiLines(WINDOW wnd, int c, PARAM p2)
{
    if (!KeyBoardMarking)    {
        if ((int)p2 & (LEFTSHIFT | RIGHTSHIFT))    { /* shift-cursor */
            switch (c)    {
                case HOME:
                case CTRL_HOME:
                case CTRL_BS:
                case PGUP:
                case CTRL_PGUP:
                case UP:
                case BS:
                case END:
                case CTRL_END:
                case PGDN:
                case CTRL_PGDN:
                case DN:
#ifdef HOOKKEYB
                case FWD:	/* right arrow! */
                case CTRL_FWD:	/* old ctrl-rightarrow */
#else
		case RARROW:	/* formerly called FWD */
		case LARROW:	/* hope that makes sense */
		case CTRL_RARROW: /* new ctrl-rightarrow */
		case CTRL_LARROW: /* ctrl-leftarrow */
#endif
                    KeyBoardMarking = TextMarking = TRUE;
                    SetAnchor(wnd, wnd->CurrCol, wnd->CurrLine);
                    break;
                default:
                    break;
            }
        }
    }
}
/* ---------- page/scroll keys ----------- */
static int DoScrolling(WINDOW wnd, int c, PARAM p2)
{
    switch (c)    {
        case PGUP:
        case PGDN:
            if (isMultiLine(wnd))
                BaseWndProc(EDITBOX, wnd, KEYBOARD, c, p2);
            break;
        case CTRL_PGUP:
        case CTRL_PGDN:
            BaseWndProc(EDITBOX, wnd, KEYBOARD, c, p2);
            break;
        case HOME:
            Home(wnd);
            break;
        case END:
            End(wnd);
            break;
#ifdef HOOKKEYB
        case CTRL_FWD:		/* old ctrl-rightarrow */
#else
        case CTRL_RARROW:	/* new ctrl-rightarrow */
#endif
            NextWord(wnd);
            break;
#ifndef HOOKKEYB
	case CTRL_LARROW:	/* ctrl-leftarrow */
#endif
        case CTRL_BS:
            PrevWord(wnd);
            break;
        case CTRL_HOME:
            if (isMultiLine(wnd))    {
                SendMessage(wnd, SCROLLDOC, TRUE, 0);
                wnd->CurrLine = 0;
                wnd->WndRow = 0;
            }
            Home(wnd);
            break;
        case CTRL_END:
			if (isMultiLine(wnd) &&
					wnd->WndRow+wnd->wtop+1 < wnd->wlines
						&& wnd->wlines > 0) {
                SendMessage(wnd, SCROLLDOC, FALSE, 0);
                SetLinePointer(wnd, wnd->wlines-1);
                wnd->WndRow =
                    min(ClientHeight(wnd)-1, wnd->wlines-1);
                Home(wnd);
            }
            End(wnd);
            break;
        case UP:
            if (isMultiLine(wnd))
                Upward(wnd);
            break;
        case DN:
            if (isMultiLine(wnd))
                Downward(wnd);
            break;
#ifdef HOOKKEYB
        case FWD: /* old name for rightarrow */
#else
	case RARROW: /* formerly called FWD */
#endif
            Forward(wnd);
            break;
#ifdef HOOKKEYB
        case BS: /* why should BackSpace do only cursor movement??? */
#else
	/* indeed: if we had BS here, BS would only move the cursor! */
	case LARROW: /* hope this makes sense */
#endif
            Backward(wnd);
            break;
        default:
            return FALSE;
    }
    if (!KeyBoardMarking && TextBlockMarked(wnd))    {
        ClearTextBlock(wnd); /* un-mark block */
        SendMessage(wnd, PAINT, 0, 0);
    }
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    return TRUE;
}
/* -------------- Del key ---------------- */
static void DelKey(WINDOW wnd)
{
    char *currchar = CurrChar;
    int repaint = *currchar == '\n';
    if (TextBlockMarked(wnd))    {
        SendMessage(wnd, COMMAND, ID_DELETETEXT, 0);
        SendMessage(wnd, PAINT, 0, 0);
        return;
    }
    if (isMultiLine(wnd) && *currchar == '\n' && *(currchar+1) == '\0')
        return;
    strcpy(currchar, currchar+1);
    if (repaint)    {
        BuildTextPointers(wnd);
        SendMessage(wnd, PAINT, 0, 0);
    }
    else    {
        ModTextPointers(wnd, wnd->CurrLine+1, -1);
        WriteTextLine(wnd, NULL, wnd->WndRow+wnd->wtop, FALSE);
    }
    wnd->TextChanged = TRUE;
}
/* ------------ Tab key ------------ */
/* not called in tab-type-through mode -ea */
static void TabKey(WINDOW wnd, PARAM p2)
{
    int insmd = wnd->InsertMode;

    if (isMultiLine(wnd))
        {
        do
            {
            char *cc = CurrChar+1;

            if (!insmd && *cc == '\0')
                break;

            if (wnd->textlen == wnd->MaxTextLength)
                break;

#ifdef HOOKKEYB
            SendMessage(wnd,KEYBOARD,insmd ? ' ' : FWD,0); /* !?!? */
#else
            SendMessage(wnd,KEYBOARD,insmd ? ' ' : RARROW,0); /* !?!? */
#endif
            }
        while (wnd->CurrCol % SysConfig.EditorTabSize);

        }
    else
        PostMessage(GetParent(wnd), KEYBOARD, '\t', p2);
}
/* ------------ Shift+Tab key ------------ */
/* inverse tab. Not called in tab-type-through mode -ea */
static void ShiftTabKey(WINDOW wnd, PARAM p2)
{
    if (isMultiLine(wnd))    {
        do  {
            if (CurrChar == GetText(wnd))
                break;
            SendMessage(wnd,KEYBOARD,BS,0);
            /* *** ^-- yet again, BS used as alias for LARROW here *** */
        } while (wnd->CurrCol % SysConfig.EditorTabSize);
    }
	else
	    PostMessage(GetParent(wnd), KEYBOARD, SHIFT_HT, p2);
}
/* --------- All displayable typed keys ------------- */
static void KeyTyped(WINDOW wnd, int c)
{
    char *currchar = CurrChar;
#ifdef HOOKKEYB
    if (c == '\0' || (c & OFFSET))
#else
    if ( (c == '\0') || ((c & FKEY) != 0) ) /* skip this if function key */
#endif
        /* ---- not recognized by editor --- */
	/* cursor and stuff already done by our caller - Eric */
	/* so I change (c != '\n' && c < ' ') to c == '\0'    */
	/* -> now we may type ESC and other stuff - Eric      */
        return;

    if ( (!isMultiLine(wnd)) && TextBlockMarked(wnd) )    {
#if 0
                { /* ****************** */
	                beep(); beep(); beep();
	                poke(0xb800,2,c);
                } /* ****************** */
#endif
	SendMessage(wnd, CLEARTEXT, 0, 0);
	/* ^-- huh? Anything typed zaps current selection contents? */
        currchar = CurrChar;
    }
    /* ---- test typing at end of text ---- */
    if (currchar == wnd->text+wnd->MaxTextLength)    {
        /* ---- typing at the end of maximum buffer ---- */
        beep();
        return;
    }
    if (*currchar == '\0')    {
        /* --- insert a newline at end of text --- */
        *currchar = '\n';
        *(currchar+1) = '\0';
        BuildTextPointers(wnd);
    }
    /* --- displayable char or newline --- */
    if (c == '\n' || wnd->InsertMode || *currchar == '\n') {
        /* ------ inserting the keyed character ------ */
        if (wnd->text[wnd->textlen-1] != '\0')    {
            /* --- the current text buffer is full --- */
            if (wnd->textlen == wnd->MaxTextLength)    {
                /* --- text buffer is at maximum size --- */
                beep();
                return;
            }
            /* ---- increase the text buffer size ---- */
            wnd->textlen += GROWLENGTH;
            /* --- but not above maximum size --- */
            if (wnd->textlen > wnd->MaxTextLength)
                wnd->textlen = wnd->MaxTextLength;
            wnd->text = DFrealloc(wnd->text, wnd->textlen+2);
            wnd->text[wnd->textlen-1] = '\0';
            currchar = CurrChar;
        }
        memmove(currchar+1, currchar, strlen(currchar)+1);
        ModTextPointers(wnd, wnd->CurrLine+1, 1);
        if (isMultiLine(wnd) && wnd->wlines > 1)
            wnd->textwidth = max(wnd->textwidth,
                (int) (TextLine(wnd, wnd->CurrLine+1)-
                TextLine(wnd, wnd->CurrLine)));
        else
            wnd->textwidth = max(wnd->textwidth,
                strlen(wnd->text));
        WriteTextLine(wnd, NULL,
            wnd->wtop+wnd->WndRow, FALSE);
    }
    /* ----- put the char in the buffer ----- */
    *currchar = c;
    wnd->TextChanged = TRUE;
    if (c == '\n')    {
        wnd->wleft = 0;
        BuildTextPointers(wnd);
        End(wnd);
        Forward(wnd);
        SendMessage(wnd, PAINT, 0, 0);
        return;
    }
    /* ---------- test end of window --------- */
    if (WndCol == ClientWidth(wnd)-1)    {
        if (!isMultiLine(wnd))	{
			if (!(currchar == wnd->text+wnd->MaxTextLength-2))
            SendMessage(wnd, HORIZSCROLL, TRUE, 0);
		}
		else	{
			char *cp = currchar;
	        while (*cp != ' ' && cp != TextLine(wnd, wnd->CurrLine))
	            --cp;
	        if (cp == TextLine(wnd, wnd->CurrLine) ||
	                !wnd->WordWrapMode)
	            SendMessage(wnd, HORIZSCROLL, TRUE, 0);
	        else    {
	            int dif = 0;
	            if (c != ' ')    {
	                dif = (int) (currchar - cp);
	                wnd->CurrCol -= dif;
	                SendMessage(wnd, KEYBOARD, DEL, 0);
	                --dif;
	            }
	            SendMessage(wnd, KEYBOARD, '\n', 0);
	            currchar = CurrChar;
	            wnd->CurrCol = dif;
	            if (c == ' ')
	                return;
	        }
	    }
	}
    /* ------ display the character ------ */
    SetStandardColor(wnd);
	if (wnd->protect)
		c = '*';
    PutWindowChar(wnd, c, WndCol, wnd->WndRow);
    /* ----- advance the pointers ------ */
    wnd->CurrCol++;
}

/* ------------ screen changing key strokes ------------- */
static void DoKeyStroke(WINDOW wnd, int c, PARAM p2)
{
    if (SysConfig.EditorGlobalReadOnly && TestAttribute(wnd, READONLY)) {
        /* read only mode added 0.7b */
        beep();
        return;
    }

    if ( ((unsigned int)p2) == SYSRQKEY )
        goto doNormalKey;	/* verbatim type mode: ^P + any key  */
        			/* see KeyboardMsg for preparations! */

    switch (c)    {
        case BS:	/* formerly called RUBOUT */
		if (wnd->CurrCol == 0 && wnd->CurrLine == 0)
			break;
#ifdef HOOKKEYB
		SendMessage(wnd, KEYBOARD, BS, 0);
#else
		SendMessage(wnd, KEYBOARD, LARROW, 0);
#endif
		/* *** ^-- to be seen as "left arrow" here! *** */
		SendMessage(wnd, KEYBOARD, DEL, 0);
		/* *** ^-- remove char at cursor *** */
		break;
        case DEL:
            DelKey(wnd);
            break;
        case SHIFT_HT:
            if (SysConfig.EditorTabSize <= 1)
                goto doNormalKey; /* tab-type-through mode -ea */
            ShiftTabKey(wnd, p2);
            break;
        case '\t':
            if (SysConfig.EditorTabSize <= 1)
                goto doNormalKey; /* tab-type-through mode -ea */
            TabKey(wnd, p2);
            break;
        case '\r':
            if (!isMultiLine(wnd))    {
                PostMessage(GetParent(wnd), KEYBOARD, c, p2);
                break;
            }
            c = '\n'; /* fall through to default case here... */
        default:
            doNormalKey:
            if ( TextBlockMarked(wnd) &&
#ifdef HOOKKEYB
                 ((c & OFFSET) == 0)
#else
                 ((c & FKEY) == 0)
#endif
                 /* F-keys should not zap current selection! (0.6c) */
               )
            {
#if 0
                { /* ****************** */
	                beep(); beep();
	                poke(0xb800,0,c);
                } /* ****************** */
#endif
                SendMessage(wnd, COMMAND, ID_DELETETEXT, 0);
                SendMessage(wnd, PAINT, 0, 0);
            } /* /typed normal key while text was selected... */
            KeyTyped(wnd, c);
            break;
    }
}

/* ----------- KEYBOARD Message ---------- */
static int KeyboardMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int c = (int) p1;
    if (WindowMoving || WindowSizing || ((int)p2 & ALTKEY))
        return FALSE;
    /* unless window is moving / resizing... */

    switch (c)    { /* all except Alt-... */
        /* we stop processing F-keys / Ctrl-... at this point... */
        /* --- these keys get processed by lower classes --- */
/* *** Allow people to type ESC as normal text while editbox has focus -ea

ASM: NO! Then the dialogs no longer process ESC as Cancel dialog

*/

        case ESC:

        case F1: /* help */
        case F2:
        case F3: /* repeat search */
        case F4:
        case F5:
        case F6:
        case F7:
        case F8: /* ... */
        case F9:
        case F10: /* open menu */
        case INS:
/*
        case SHIFT_INS:
        case SHIFT_DEL:
*/
            return FALSE;	/* for all keys which are NOT for us! */
            /* (all other keys will be HIDDEN from more generic classes!) */
        /* --- these keys get processed here --- */
#ifdef HOOKKEYB
        case CTRL_FWD: /* ctrl-rightarrow */
#else
	case CTRL_RARROW:
	case CTRL_LARROW:
#endif
        case CTRL_BS:
        case CTRL_HOME:
        case CTRL_END:
        case CTRL_PGUP:
        case CTRL_PGDN:
            break;

        case CTRL_N:	/* 0.7a NEW  file */
        case CTRL_O:	/* 0.7a OPEN file */
        case CTRL_S:	/* 0.7a SAVE file */
        case CTRL_F:	/* 0.7c FIND text */
        case ALT_1: case ALT_2: case ALT_3:
        case ALT_4: case ALT_5: case ALT_6:
        case ALT_7: case ALT_8: case ALT_9:	/* 0.7c goto window */
            return FALSE; /* bounce to lower classes (for menu items) ... */
            		/* ... so they are not avail for verbatim typing  */

	/* *** force processing those elsewhere even if CTRL not really *** */
	/* *** pressed, for shift-ins/del emulation of ctrl-v/x paste/  *** */
	/* *** cut and shift-shift-ins ctrl-c copy emulation...         *** */
	case CTRL_V:	/* for PASTE */
	case CTRL_X:	/* for CUT   */
	case CTRL_C:	/* for COPY  */
	case CTRL_Z:	/* for UNDO  */
	case CTRL_F4:	/* for close */
	    {
	        BOOL tmb = TextMarking;
	        if ( ((int)p2 & (CTRLKEY|ALTKEY|LEFTSHIFT|RIGHTSHIFT)) == 0) {
	            p2 = SYSRQKEY; 	/* CTRL-... but CTRL/SHIFT/ALT all not  */
	            break;		/* pressed -> must be Alt-digit typing! */
	        }
	        StopMarking(wnd); /* *** ensure un-swapped selection endpoints *** */
	        TextMarking = tmb;
	    }
            return FALSE; /* bounce keyboard message back to lower classes */

	case CTRL_P:	/* CTRL-P -> fetch 1 more key for verbatim typing */
#ifdef FANCY_CTRL_P
	    SendMessage(GetParent(wnd), ADDSTATUS,
	        (PARAM) "^P: Press any key for verbatim insertion.", 0);
	    while (!keyhit())
	        dispatch_message();	/* let (nested!) messages flow */
#endif	        /* *** if this ever crashes, remove the dispatch_message() call! *** */
	    c = getkey() & 0xff;	/* not elegant, of course! */
	    p2 = SYSRQKEY;		/* magic shift status */
#ifdef FANCY_CTRL_P
            SendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow); /* normal status bar again */
#endif
	    break;

        default:
            /* other ctrl keys get processed by lower classes */
            if ((int)p2 & CTRLKEY)
#ifndef DISABLE_TYPING_CTRL_KEYS
	      /* we enumerated all USED CTRL keys above.  */
	      /* Others may be typed as text from now on. */
	      if (p1 > CTRL_Z) /* CTRL_A ... CTRL_Z are ASCII 1..26 */
                return FALSE;
#endif
            /* --- all other keys get processed here --- */
            break;
    }
    /* only reached if: no Alt-key, no F-key, no USED Ctrl-key */

    if (p2 == SYSRQKEY) { /* special "type verbatim" mode */
        DoKeyStroke(wnd, c, p2);
        SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
        return TRUE;	/* consume key event */
    }

    DoMultiLines(wnd, c, p2);
    if (DoScrolling(wnd, c, p2)) {
        if (KeyBoardMarking)
            ExtendBlock(wnd, WndCol, wnd->WndRow);
    }
    else if (!TestAttribute(wnd, READONLY)) {
        DoKeyStroke(wnd, c, p2);
        SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    }
    else if (c == '\t')
        PostMessage(GetParent(wnd), KEYBOARD, '\t', p2);
    else
	beep(); /* readonly and tried to really type something */
    return TRUE;
}

/* ----------- SHIFT_CHANGED Message ---------- */
static void ShiftChangedMsg(WINDOW wnd, PARAM p1)
{
    if ( !( (int)p1 & (LEFTSHIFT | RIGHTSHIFT) ) &&
         KeyBoardMarking) {
        StopMarking(wnd);
        KeyBoardMarking = FALSE;
    }
}

/* ----------- ID_DELETETEXT Command ---------- */
static void DeleteTextCmd(WINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bb;
        char *be;
        unsigned int len;
        BOOL tmb = TextMarking;
        StopMarking(wnd); /* swap marks if begin was after end */
        TextMarking = tmb;
        bb = TextBlockBegin(wnd);
        be = TextBlockEnd(wnd);
        if (bb == be)
            return;	/* empty deletion */
        if (bb >= be) {	/* new check 0.7c */
            bb = TextBlockEnd(wnd);	/* sic! */
            be = TextBlockBegin(wnd);	/* sic! */
        }
        len = (unsigned int) (be - bb); /* *** unsigned *** */
        SaveDeletedText(wnd, bb, len);
        wnd->TextChanged = TRUE;
        strcpy(bb, be);	/* copy text after deletion over deleted text */
        wnd->CurrLine = TextLineNumber(wnd, bb-wnd->BlkBegCol);	/* ?? */
        wnd->CurrCol = wnd->BlkBegCol;
        wnd->WndRow = wnd->BlkBegLine - wnd->wtop;
        if (wnd->WndRow < 0)    {
            wnd->wtop = wnd->BlkBegLine;
            wnd->WndRow = 0;
        }
        SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
        ClearTextBlock(wnd);
        BuildTextPointers(wnd);
    }
}

/* ----------- ID_CLEAR Command ---------- */
static void ClearCmd(WINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bb;
        char *be;
        unsigned int len;
        BOOL tmb = TextMarking;
        StopMarking(wnd); /* swap marks if begin was after end */
        TextMarking = tmb;
        bb = TextBlockBegin(wnd);
        be = TextBlockEnd(wnd);
        if (bb == be)
            return;	/* empty deletion */
        if (bb >= be) {	/* new check 0.7c */
            bb = TextBlockEnd(wnd);	/* sic! */
            be = TextBlockBegin(wnd);	/* sic! */
        }
        len = (unsigned int) (be - bb); /* *** unsigned *** */
        SaveDeletedText(wnd, bb, len);
        wnd->CurrLine = TextLineNumber(wnd, bb);
        wnd->CurrCol = wnd->BlkBegCol;
        wnd->WndRow = wnd->BlkBegLine - wnd->wtop;
        if (wnd->WndRow < 0)    {
            wnd->WndRow = 0;
            wnd->wtop = wnd->BlkBegLine;
        }
        /* ------ change all text lines in block to \n ----- */
        while (bb < be)    {
            char *cp = strchr(bb, '\n');
            if (cp > be)
                cp = be;
            strcpy(bb, cp);
            be -= (int) (cp - bb);	/* is that safe to do? */
            bb++;
        }
        ClearTextBlock(wnd);
        BuildTextPointers(wnd);
        SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
        wnd->TextChanged = TRUE;
    }
}

/* ----------- ID_UNDO Command ---------- */
static void UndoCmd(WINDOW wnd)
{
    if (wnd->DeletedText != NULL)    {
        PasteText(wnd, wnd->DeletedText, wnd->DeletedLength);
        free(wnd->DeletedText);
        wnd->DeletedText = NULL;
        wnd->DeletedLength = 0;
        SendMessage(wnd, PAINT, 0, 0);
    }
}

/* ----------- ID_PARAGRAPH Command ---------- */
static void ParagraphCmd(WINDOW wnd)
{
    int bc, fl;
    char *bl, *bbl, *bel, *bb;

    ClearTextBlock(wnd);
    /* ---- forming paragraph from cursor position --- */
    fl = wnd->wtop + wnd->WndRow;
    bbl = bel = bl = TextLine(wnd, wnd->CurrLine);
    if ((bc = wnd->CurrCol) >= ClientWidth(wnd))
        bc = 0;
    Home(wnd);
    /* ---- locate the end of the paragraph ---- */
    while (*bel)    {
        int blank = TRUE;
        char *bll = bel;
        /* --- blank line marks end of paragraph --- */
        while (*bel && *bel != '\n')    {
            if (*bel != ' ')
                blank = FALSE;
            bel++;
        }
        if (blank)    {
            bel = bll;
            break;
        }
        if (*bel)
            bel++;
    }
    if (bel == bbl)    {
        SendMessage(wnd, KEYBOARD, DN, 0);
        return;
    }
    if (*bel == '\0')
        --bel;
    if (*bel == '\n')
        --bel;
    /* --- change all newlines in block to spaces --- */
    while (CurrChar < bel)    {
        if (*CurrChar == '\n')    {
            *CurrChar = ' ';
            wnd->CurrLine++;
            wnd->CurrCol = 0;
        }
        else
            wnd->CurrCol++;
    }
    /* ---- insert newlines at new margin boundaries ---- */
    bb = bbl;
    while (bbl < bel)    {
        bbl++;
        if ((int)(bbl - bb) == ClientWidth(wnd)-1)    {
            while (*bbl != ' ' && bbl > bb)
                --bbl;
            if (*bbl != ' ')    {
                bbl = strchr(bbl, ' ');
                if (bbl == NULL || bbl >= bel)
                    break;
            }
            *bbl = '\n';
            bb = bbl+1;
        }
    }
    BuildTextPointers(wnd);
    /* --- put cursor back at beginning --- */
    wnd->CurrLine = TextLineNumber(wnd, bl);
    wnd->CurrCol = bc;
    if (fl < wnd->wtop)
        wnd->wtop = fl;
    wnd->WndRow = fl - wnd->wtop;
    SendMessage(wnd, PAINT, 0, 0);
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    wnd->TextChanged = TRUE;
    BuildTextPointers(wnd);
}

/* ----------- COMMAND Message ---------- */
static int CommandMsg(WINDOW wnd, PARAM p1)
{
    if (SysConfig.EditorGlobalReadOnly && TestAttribute(wnd, READONLY)) {
        /* read only mode added 0.7b */
        switch ((int)p1) {
            case ID_REPLACE:
            case ID_CUT:
            case ID_PASTE:
            case ID_DELETETEXT:
            case ID_CLEAR:
            case ID_PARAGRAPH:
            case ID_UPCASE:	/* new readonly mode handling for 0.7d */
            case ID_DOWNCASE:
                beep();
                return TRUE;	/* consume event */
        }
    }

    switch ((int)p1)    {
		case ID_SEARCH:
			SearchText(wnd);
			return TRUE;
		case ID_REPLACE:
			ReplaceText(wnd);
			return TRUE;
		case ID_SEARCHNEXT:
			SearchNext(wnd);
			return TRUE;
		case ID_CUT:
			CopyToClipboard(wnd);
			SendMessage(wnd, COMMAND, ID_DELETETEXT, 0);
			SendMessage(wnd, PAINT, 0, 0);
			return TRUE;
		case ID_COPY:
			CopyToClipboard(wnd);
			ClearTextBlock(wnd);
			SendMessage(wnd, PAINT, 0, 0);
			return TRUE;
		case ID_PASTE:
			PasteFromClipboard(wnd);
			SendMessage(wnd, PAINT, 0, 0);
			return TRUE;
        case ID_DELETETEXT:
            DeleteTextCmd(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;
        case ID_CLEAR:
            ClearCmd(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;
        case ID_UNDO:
            UndoCmd(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;
        case ID_PARAGRAPH:
            ParagraphCmd(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;

        /* 0.7d added commands follow (7/2005) */
        case ID_UPCASE:
            UpCaseMarked(wnd);
            ClearTextBlock(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;
        case ID_DOWNCASE:
            DownCaseMarked(wnd);
            ClearTextBlock(wnd);
            SendMessage(wnd, PAINT, 0, 0);
            return TRUE;
        case ID_WORDCOUNT:
            {
                unsigned bytes, words, lines;
                char statsline[50];
                statsline[0] = 0;
                StatsForMarked(wnd, &bytes, &words, &lines);
                ClearTextBlock(wnd);
                SendMessage(wnd, PAINT, 0, 0);
                sprintf(statsline," %u lines, %u words, %u bytes ",
                    lines, words, bytes);
	        SendMessage(GetParent(wnd), ADDSTATUS,
	            (PARAM) statsline, 0);
#if 0
!	        while (!keyhit())
!	            dispatch_message();	/* let (nested!) messages flow */
#endif
	    }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

/* ---------- CLOSE_WINDOW Message ----------- */
static int CloseWindowMsg(WINDOW wnd, PARAM p1, PARAM p2)
{
    int rtn;
    SendMessage(NULL, HIDE_CURSOR, 0, 0);
    if (wnd->DeletedText != NULL) {
        free(wnd->DeletedText);
        wnd->DeletedText = NULL;
    }
    rtn = BaseWndProc(EDITBOX, wnd, CLOSE_WINDOW, p1, p2);
    if (wnd->text != NULL) {
	free(wnd->text);
	wnd->text = NULL;
    }
    return rtn;
}

/* ------- Window processing module for EDITBOX class ------ */
int EditBoxProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn;
    switch (msg)    {
        case CREATE_WINDOW:
            return CreateWindowMsg(wnd);
        case ADDTEXT:
            return AddTextMsg(wnd, p1, p2);
        case SETTEXT:
            return SetTextMsg(wnd, p1);
        case CLEARTEXT:
	    return ClearTextMsg(wnd);
        case GETTEXT:
            return GetTextMsg(wnd, p1, p2);
        case SETTEXTLENGTH:
            return SetTextLengthMsg(wnd, (unsigned) p1);
        case KEYBOARD_CURSOR:
            KeyboardCursorMsg(wnd, p1, p2);
	    return TRUE;
        case SETFOCUS:
	    if (!(int)p1)
	    SendMessage(NULL, HIDE_CURSOR, 0, 0);
        case PAINT:
        case MOVE:
            rtn = BaseWndProc(EDITBOX, wnd, msg, p1, p2);
            SendMessage(wnd,KEYBOARD_CURSOR,WndCol,wnd->WndRow);
            return rtn;
        case SIZE:
            return SizeMsg(wnd, p1, p2);
        case SCROLL:
            return ScrollMsg(wnd, p1);
        case HORIZSCROLL:
            return HorizScrollMsg(wnd, p1);
        case SCROLLPAGE:
            return ScrollPageMsg(wnd, p1);
        case HORIZPAGE:
            return HorizPageMsg(wnd, p1);
        case LEFT_BUTTON:
            if (LeftButtonMsg(wnd, p1, p2))
                return TRUE;
            break;
        case MOUSE_MOVED:
            if (MouseMovedMsg(wnd, p1, p2))
                return TRUE;
            break;
        case BUTTON_RELEASED:
            if (ButtonReleasedMsg(wnd))
                return TRUE;
            break;
        case KEYBOARD:
            if (KeyboardMsg(wnd, p1, p2))
                return TRUE;
            break;
        case SHIFT_CHANGED:
            ShiftChangedMsg(wnd, p1);
            break;
        case COMMAND:
            if (CommandMsg(wnd, p1))
                return TRUE;
            break;
        case CLOSE_WINDOW:
            return CloseWindowMsg(wnd, p1, p2);
        default:
            break;
    }
    return BaseWndProc(EDITBOX, wnd, msg, p1, p2);
}

/* ------ save deleted text for the Undo command ------ */
static void SaveDeletedText(WINDOW wnd, char *bbl, unsigned int len)
{
    /* removed "if len > 5000 or even > 0x8000" check in 0.7c, */
    /* hopefully you CAN save/undelete > 32k chars properly... */

    wnd->DeletedLength = len; /* *** UNSIGNED! *** */
    wnd->DeletedText=DFrealloc(wnd->DeletedText,len);
    memmove(wnd->DeletedText, bbl, len);
}

/* ---- cursor right key: right one character position ---- */
static void Forward(WINDOW wnd)
{
    char *cc = CurrChar+1;
    if (*CurrChar == '\0')
        return;
    if (*CurrChar == '\n')    {
        if (*cc != '\0') {
            Home(wnd);
            Downward(wnd);
        }
    }
    else    {
        wnd->CurrCol++;
        if (WndCol == ClientWidth(wnd))
            SendMessage(wnd, HORIZSCROLL, TRUE, 0);
    }
}

/* ----- stick the moving cursor to the end of the line ---- */
static void StickEnd(WINDOW wnd)
{
    char *cp = TextLine(wnd, wnd->CurrLine);
    char *cp1 = strchr(cp, '\n');
    int len = cp1 ? (int) (cp1 - cp) : 0;
    wnd->CurrCol = min(len, wnd->CurrCol);
    if (wnd->wleft > wnd->CurrCol)    {
        wnd->wleft = max(0, wnd->CurrCol - 4);
        SendMessage(wnd, PAINT, 0, 0);
    }
    else if (wnd->CurrCol-wnd->wleft >= ClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (ClientWidth(wnd)-1);
        SendMessage(wnd, PAINT, 0, 0);
    }
}

/* --------- cursor down key: down one line --------- */
static void Downward(WINDOW wnd)
{
    if (isMultiLine(wnd) &&
            wnd->WndRow+wnd->wtop+1 < wnd->wlines)  {
        wnd->CurrLine++;
        if (wnd->WndRow == ClientHeight(wnd)-1)
			SendMessage(wnd, SCROLL, TRUE, 0);
        wnd->WndRow++;
        StickEnd(wnd);
    }
}

/* -------- cursor up key: up one line ------------ */
static void Upward(WINDOW wnd)
{
    if (isMultiLine(wnd) && wnd->CurrLine != 0)    {
        --wnd->CurrLine;
        if (wnd->WndRow == 0)
			SendMessage(wnd, SCROLL, FALSE, 0);
        --wnd->WndRow;
        StickEnd(wnd);
    }
}

/* ---- cursor left key: left one character position ---- */
static void Backward(WINDOW wnd)
{
    if (wnd->CurrCol)    {
        --wnd->CurrCol;
        if (wnd->CurrCol < wnd->wleft)
            SendMessage(wnd, HORIZSCROLL, FALSE, 0);
    }
    else if (isMultiLine(wnd) && wnd->CurrLine != 0)    {
        Upward(wnd);
        End(wnd);
    }
}

/* -------- End key: to end of line ------- */
static void End(WINDOW wnd)
{
    while (*CurrChar && *CurrChar != '\n')
        ++wnd->CurrCol;
    if (WndCol >= ClientWidth(wnd))    {
        wnd->wleft = wnd->CurrCol - (ClientWidth(wnd)-1);
        SendMessage(wnd, PAINT, 0, 0);
    }
}

/* -------- Home key: to beginning of line ------- */
static void Home(WINDOW wnd)
{
    wnd->CurrCol = 0;
    if (wnd->wleft != 0)    {
        wnd->wleft = 0;
        SendMessage(wnd, PAINT, 0, 0);
    }
}

/* -- Ctrl+cursor right key: to beginning of next word -- */
static void NextWord(WINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    ClearVisible(wnd);
    while (!isWhite(*CurrChar))    {
        char *cc = CurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    while (isWhite(*CurrChar))    {
        char *cc = CurrChar+1;
        if (*cc == '\0')
            break;
        Forward(wnd);
    }
    SetVisible(wnd);
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        SendMessage(wnd, PAINT, 0, 0);
}

/* -- Ctrl+cursor left key: to beginning of previous word -- */
static void PrevWord(WINDOW wnd)
{
    int savetop = wnd->wtop;
    int saveleft = wnd->wleft;
    ClearVisible(wnd);
    Backward(wnd);
    while (isWhite(*CurrChar))    {
        if (wnd->CurrLine == 0 && wnd->CurrCol == 0)
            break;
        Backward(wnd);
    }
    while (wnd->CurrCol != 0 && !isWhite(*CurrChar))
        Backward(wnd);
    if (isWhite(*CurrChar))
        Forward(wnd);
    SetVisible(wnd);
    if (wnd->wleft != saveleft)
        if (wnd->CurrCol >= saveleft)
            if (wnd->CurrCol - saveleft < ClientWidth(wnd))
                wnd->wleft = saveleft;
    SendMessage(wnd, KEYBOARD_CURSOR, WndCol, wnd->WndRow);
    if (wnd->wtop != savetop || wnd->wleft != saveleft)
        SendMessage(wnd, PAINT, 0, 0);
}

/* ----- modify text pointers from a specified position
                by a specified plus or minus amount ----- */
static void ModTextPointers(WINDOW wnd, int lineno, int var)
{
    while (lineno < wnd->wlines)
        *((wnd->TextPointers) + lineno++) += var;
}

/* ----- set anchor point for marking text block ----- */
static void SetAnchor(WINDOW wnd, int mx, int my)
{
    ClearTextBlock(wnd);
    /* ------ set the anchor ------ */
    wnd->BlkBegLine = wnd->BlkEndLine = my;
    wnd->BlkBegCol = wnd->BlkEndCol = mx;
    SendMessage(wnd, PAINT, 0, 0);
}


/* NEW 7/2005 - not actually clipboard related but editbox.c is */
/* already tooo long anyway ;-) In-place text section stuff...  */
/* toupper/tolower: see ctype.h  -  do they support COUNTRY...? */

void UpCaseMarked(WINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bb = TextBlockBegin(wnd);	/* near pointers */
        char *be = TextBlockEnd(wnd);	/* near pointers */
        if (bb >= be) {
            bb = TextBlockEnd(wnd);	/* sic! */
            be = TextBlockBegin(wnd);	/* sic! */
        }
        while (bb < be) {
           bb[0] = toupper(bb[0]);
           bb++;
        }
    }
}

void DownCaseMarked(WINDOW wnd)
{
    if (TextBlockMarked(wnd))    {
        char *bb = TextBlockBegin(wnd);	/* near pointers */
        char *be = TextBlockEnd(wnd);	/* near pointers */
        if (bb >= be) {
            bb = TextBlockEnd(wnd);	/* sic! */
            be = TextBlockBegin(wnd);	/* sic! */
        }
        while (bb < be) {
           bb[0] = tolower(bb[0]);
           bb++;
        }
    }
}

void StatsForMarked(WINDOW wnd, unsigned *bytes, unsigned *words, unsigned *lines)
{
    bytes[0] = words[0] = lines[0] = 0;
    if (TextBlockMarked(wnd))    {
    	int inWord = 0;
        char *bb = TextBlockBegin(wnd);	/* near pointers */
        char *be = TextBlockEnd(wnd);	/* near pointers */
        if (bb >= be) {
            bb = TextBlockEnd(wnd);	/* sic! */
            be = TextBlockBegin(wnd);	/* sic! */
        }
        while (bb < be) {
           char c = bb[0];
           if (c == '\n') lines[0]++;
           if (isspace(c)) {
           	if (inWord) words[0]++;
           	inWord = 0;
           } else {
           	inWord = 1;
           }
           bytes[0]++;
           bb++;
        }
        if (inWord) words[0]++;
    }
}


