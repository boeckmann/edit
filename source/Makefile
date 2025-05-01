# Watcom Makefile for FreeDOS EDIT
.erase

CFLAGS=-q -ml
!ifdef DEBUG
CFLAGS += -d2
!else
CFLAGS += -os
!endif

objs = &
	main.obj &
	edit.obj &
	applicat.obj &
	asciitab.obj &
	box.obj &
	button.obj &
	calendar.obj &
	checkbox.obj &
	clipbord.obj &
	combobox.obj &
	commdlg.obj &
	config.obj &
	console.obj &
	decomp.obj &
	dfalloc.obj &
	dialbox.obj &
	direct.obj &
	editbox.obj &
	helpbox.obj &
	htree.obj &
	keys.obj &
	listbox.obj &
	lists.obj &
	menu.obj &
	menubar.obj &
	menus.obj &
	message.obj &
	mouse.obj &
	msgbox.obj &
	normal.obj &
	pictbox.obj &
	popdown.obj &
	portab.obj &
	radio.obj &
	rect.obj &
	search.obj &
	slidebox.obj &
	spinbutt.obj &
	statbar.obj &
	syshelp.obj &
	sysmenu.obj &
	systemrs.obj &
	text.obj &
	textbox.obj &
	video.obj &
	watch.obj &
	window.obj


!ifdef LOG
objs +=	logger.obj
#objs += log.obj &
!else
objs += nolog.obj
!endif
	
all: edit.exe

edit.exe: $(objs) edit.lnk Makefile
	wlink @edit.lnk

.c.obj:
	wcc $(CFLAGS) -Fo=$*.obj $*.c

edit.lnk : Makefile
	@echo NAME $^& >$^@
	@echo SYSTEM DOS >>$^@
	@echo OPTION QUIET >>$^@
	@echo OPTION MAP >>$^@
	@echo OPTION STACK=8192 >>$^@
!ifdef DEBUG
	@echo DEBUG ALL >>$^@
!endif
	@for %i in ($(objs)) do @echo FILE %i >>$^@

clean: .symbolic
	rm -f *.obj
	rm -f *.exe
	rm -f *.err
