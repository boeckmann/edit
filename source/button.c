/* -------------- button.c -------------- */

#include "dfpcomp.h"

void PaintMsg(WINDOW wnd, CTLWINDOW *ct, RECT *rc)
{

    if (isVisible(wnd))    {

        if (TestAttribute(wnd, SHADOW) && !SysConfig.VideoCurrentColorScheme.isMonoScheme)    {
            /* -------- draw the button's shadow ------- */
            int x;
            RECT rcb;
            rcb.lf = GetLeft(wnd);
            rcb.rt = GetLeft(wnd)+WindowWidth(wnd);
            rcb.tp = GetTop(wnd);
            rcb.bt = rcb.tp+1;

            background = WndBackground(GetParent(wnd));
            foreground = BLACK;
            hide_mousecursor_in_rect(rcb);
            for (x = 1; x <= WindowWidth(wnd); x++)
                wputch(wnd, 223, x, 1);
            wputch(wnd, 220, WindowWidth(wnd), 0);
            show_mousecursor();
        }
        if (ct->itext != NULL)    {
            unsigned char *txt;
            txt = DFcalloc(1, strlen(ct->itext)+10);
            if (ct->setting == OFF)    {
                txt[0] = CHANGECOLOR;
                txt[1] = wnd->WindowColors
                            [HILITE_COLOR] [FG] | 0x80;
                txt[2] = wnd->WindowColors
                            [STD_COLOR] [BG] | 0x80;
            }
            CopyCommand(txt+strlen(txt),ct->itext,!ct->setting,
                WndBackground(wnd));
            SendMessage(wnd, CLEARTEXT, 0, 0);
            SendMessage(wnd, ADDTEXT, (PARAM) txt, 0);
            free(txt);
        }
        /* --------- write the button's text ------- */
        WriteTextLine(wnd, rc, 0, wnd == inFocus);
    }
}

void LeftButtonMsg(WINDOW wnd, MESSAGE msg, CTLWINDOW *ct)
{
    if (!SysConfig.VideoCurrentColorScheme.isMonoScheme)    {
        /* --------- draw a pushed button -------- */
        int x;
        background = WndBackground(GetParent(wnd));
        foreground = WndBackground(wnd);
        hide_mousecursor();
        wputch(wnd, ' ', 0, 0);
        for (x = 0; x < WindowWidth(wnd); x++)    {
            wputch(wnd, 220, x+1, 0);
            wputch(wnd, 223, x+1, 1);
        }
        show_mousecursor();
    }
    if (msg == LEFT_BUTTON)
        SendMessage(NULL, WAITMOUSE, 0, 0);
    else
        SendMessage(NULL, WAITKEYBOARD, 0, 0);
    SendMessage(wnd, PAINT, 0, 0);
    if (ct->setting == ON)
        PostMessage(GetParent(wnd), COMMAND, ct->command, 0);
    else
        beep();
}

int ButtonProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    CTLWINDOW *ct = GetControl(wnd);

    if (ct != NULL)    {
        switch (msg)    {
            case SETFOCUS:
                BaseWndProc(BUTTON, wnd, msg, p1, p2);
                p1 = 0;
                /* ------- fall through ------- */
            case PAINT:
                PaintMsg(wnd, ct, (RECT*)p1);
                return TRUE;
            case KEYBOARD:
								if ((p1 == LARROW) || (p1 == RARROW)) { /* DFlat+ 1.0: allow buttons to pass arrows to parent*/
								   PostMessage ( GetParent(wnd), msg, p1, p2);
								   return TRUE;
								}
                if (p1 != ENTER)
                    break;
                /* ---- fall through ---- */
            case LEFT_BUTTON:
                LeftButtonMsg(wnd, msg, ct);
                return TRUE;
            case HORIZSCROLL:
                return TRUE;
            default:
                break;
        }
    }
    return BaseWndProc(BUTTON, wnd, msg, p1, p2);
}
