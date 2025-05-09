/* --------------- lists.c -------------- */

#include "dfpcomp.h"

/* ----- set focus to the next sibling ----- */
void SetNextFocus()
{
    if (inFocus != NULL)    {
        WINDOW wnd1 = inFocus, pwnd;
        while (TRUE)    {
			pwnd = GetParent(wnd1);
            if (NextWindow(wnd1) != NULL)
				wnd1 = NextWindow(wnd1);
			else if (pwnd != NULL)
                wnd1 = FirstWindow(pwnd);
            if (wnd1 == NULL || wnd1 == inFocus)	{
				wnd1 = pwnd;
				break;
			}
			if (GetClass(wnd1) == STATUSBAR || GetClass(wnd1) == MENUBAR)
				continue;
            if (isVisible(wnd1))
                break;
        }
        if (wnd1 != NULL)	{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
            if (wnd1->condition != ISCLOSING)
	            SendMessage(wnd1, SETFOCUS, TRUE, 0);
		}
    }
}

/* ----- set focus to the previous sibling ----- */
void SetPrevFocus()
{
    if (inFocus != NULL)    {
        WINDOW wnd1 = inFocus, pwnd;
        while (TRUE)    {
			pwnd = GetParent(wnd1);
            if (PrevWindow(wnd1) != NULL)
				wnd1 = PrevWindow(wnd1);
			else if (pwnd != NULL)
                wnd1 = LastWindow(pwnd);
            if (wnd1 == NULL || wnd1 == inFocus)	{
				wnd1 = pwnd;
				break;
			}
			if (GetClass(wnd1) == STATUSBAR)
				continue;
            if (isVisible(wnd1))
                break;
        }
        if (wnd1 != NULL)	{
			while (wnd1->childfocus != NULL)
				wnd1 = wnd1->childfocus;
            if (wnd1->condition != ISCLOSING)
	            SendMessage(wnd1, SETFOCUS, TRUE, 0);
		}
    }
}

/* ------- move a window to the end of its parents list ----- */
/* Bad: This shuffles around the window list items, but we need */
/* it as the "next" relation of siblings determines the window  */
/* stacking: Z coordinate, "last" window is the topmost one...  */
void ReFocus(WINDOW wnd)
{
	if (GetParent(wnd) != NULL)	{
		RemoveWindow(wnd); /* splice out window from list:    */
		/* if it was a first-child, make the next one first,  */
		/* if it was a last-child, make the previous one last */
		AppendWindow(wnd); /* add window as last one in list: */
		/* if it has a parent w/o first-child, make it first, */
		/* if it has a parent, make it last-child...          */
		ReFocus(GetParent(wnd)); /* recurse until at top */
	}
}

/* ---- remove a window from the linked list ---- */
void RemoveWindow(WINDOW wnd)
{
    if (wnd != NULL)    {
		WINDOW pwnd = GetParent(wnd);
        if (PrevWindow(wnd) != NULL)
            NextWindow(PrevWindow(wnd)) = NextWindow(wnd);
        if (NextWindow(wnd) != NULL)
            PrevWindow(NextWindow(wnd)) = PrevWindow(wnd);
		if (pwnd != NULL)	{
        	if (wnd == FirstWindow(pwnd))
            	FirstWindow(pwnd) = NextWindow(wnd);
        	if (wnd == LastWindow(pwnd))
            	LastWindow(pwnd) = PrevWindow(wnd);
		}
    }
}

/* ---- append a window to the linked list ---- */
void AppendWindow(WINDOW wnd)
{
    if (wnd != NULL)    {
		WINDOW pwnd = GetParent(wnd);
		if (pwnd != NULL)	{
        	if (FirstWindow(pwnd) == NULL)
            	FirstWindow(pwnd) = wnd;
        	if (LastWindow(pwnd) != NULL)
            	NextWindow(LastWindow(pwnd)) = wnd;
        	PrevWindow(wnd) = LastWindow(pwnd);
	        LastWindow(pwnd) = wnd;
		}
        NextWindow(wnd) = NULL;
    }
}

/* ----- if document windows and statusbar or menubar get the focus,
              pass it on ------- */
void SkipApplicationControls(void)
{
	BOOL EmptyAppl = FALSE;
	int ct = 0;
	while (!EmptyAppl && inFocus != NULL)	{
		CLASS cl = GetClass(inFocus);
		if (cl == MENUBAR || cl == STATUSBAR)	{
			SetPrevFocus();
			EmptyAppl = (cl == MENUBAR && ct++);
		}
		else
			break;
	}
}

