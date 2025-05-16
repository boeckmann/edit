/* ------------- dflat.h ----------- */
#ifndef PORTAB
#define PORTAB

/* //////////////////////////////// OPEN WATCOM ///////////////////////////// */

#ifdef __TURBOC__

typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;

#else

#include <stdint.h>
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

#endif

#if MSC | __WATCOMC__
#include <direct.h>
#include <dos.h>
#endif


#ifdef __WATCOMC__

#undef MK_FP
#define MK_FP(seg,ofs) 	(void far *)(((unsigned long)(seg)<<16)|((unsigned short)ofs))

#define MAXDRIVE 3
#define MAXPATH  80 
#define MAXDIR   66
#define MAXFILE  9
#define MAXEXT   5
/* MAXFILE is not the same as NAME_MAX=11 */

#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10


void disable(void);
#pragma aux disable = "cli" modify exact [];
void enable(void);
#pragma aux enable = "sti" modify exact [];
void halt(void);
#pragma aux halt = "hlt" modify exact [];

#define pokeb(seg, ofs, b) (*(unsigned char far *)MK_FP(seg,ofs) = (b))
#define poke(seg, ofs, w) (*(unsigned far *)MK_FP(seg,ofs) = (w))
#define pokew poke
#define pokel(seg, ofs, l) (*(unsigned long far *)MK_FP(seg,ofs) = (l))
#define peekb(seg, ofs) (*(unsigned char far *)MK_FP(seg,ofs))
#define peek(seg, ofs) (*(unsigned far *)MK_FP(seg,ofs))
#define peekw peek
#define peekl(seg, ofs) (*(unsigned long far *)MK_FP(seg,ofs))

//unsigned total_drives_;

#define getdisk()   ( _getdrive() -1 )
#define setdisk(i)  ( _dos_setdrive (i+1, NULL) )

#define setvect(n, h)  ( _dos_setvect (n,h) )

#define fnsplit(f, d, n, nm, e)  (_splitpath(f, d, n, nm, e) )
#define fnmerge(spath, drive, dir, name, ext)  (_makepath(spath, drive, dir, name, ext))

#define CFlag(regs) ( regs.x.cflag )

typedef struct fblock { struct find_t ff; } FBLOCK;
#define FindFirst(path,attr,blk) ( _dos_findfirst(path, attr, &(blk.ff)) )
#define FindNext(blk) ( _dos_findnext(&blk.ff) )
#define NameOf(blk) (blk.ff.name)
#define AttribOf(blk) (blk.ff.attrib)

void setcbrk (char check);			/* 1 = all / 0 = only con calls check */

#define  OLDCRITERR 1

void  SetControlBreakHandler(int (*fptr)());
void  RestoreControlBreakHandler ();  


#endif

/* //////////////////////////////// BORLAND/TURBO C  //////////////////////// */

#ifdef __TURBOC__

#include <dos.h>
#include <dir.h>

#define CFlag(regs) ( regs.x.flags & 0x01 )

typedef struct fblock { struct ffblk ff; } FBLOCK;
#define FindFirst(path,attr,blk) ( findfirst(path, &blk.ff, attr) )
#define FindNext(blk) ( findnext(&blk.ff) )
#define NameOf(blk) (blk.ff.ff_name)
#define AttribOf(blk) (blk.ff.ff_attrib)

#undef _A_NORMAL
#define _A_NORMAL 0x00

#define SetControlBreakHandler(f)     ctrlbrk(f)
#define RestoreControlBreakHandler()  {}


#endif

#ifdef __GNUC__

#define MAXPATH 1024

#endif

void cpu_hlt(void);

#endif

