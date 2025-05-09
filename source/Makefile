# Watcom Makefile for FreeDOS EDIT
.erase

CFLAGS=-q -ml
!ifdef DEBUG
CFLAGS += -d2
!else
CFLAGS += -os
!endif

!ifdef __UNIX__
HOST_CL=cc
REL_PATH=./
!else
HOST_CL=owcc
REL_PATH=
!endif
!ifdef __MSDOS__
HOST_CL_FLAGS=-mcmodel=l
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
	
all: edit.exe edit.hlp

edit.exe: $(objs) edit.lnk Makefile
	wlink @edit.lnk

edit.hlp: huffc.exe fixhelp.exe
	$(REL_PATH)huffc.exe edit.txt edit.hlp
	$(REL_PATH)fixhelp.exe edit

huffc.exe:
	$(HOST_CL) $(HOST_CL_FLAGS) huffc.c htree.c -o huffc.exe

fixhelp.exe:
	$(HOST_CL) $(HOST_CL_FLAGS) fixhelp.c decomp.c -o fixhelp.exe

.c.obj: .autodepend
	wcc $(CFLAGS) -Fo=$*.obj $*.c

edit.lnk : Makefile .always
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
	rm -f *.OBJ
	rm -f *.EXE
	rm -f *.err
	rm -f *.ERR
