/* ------------------- htree.h -------------------- */

#ifndef HTREE_H
#define HTREE_H

#include <stdio.h>
#include "portab.h"
typedef u16 BYTECOUNTER;

/* ---- Huffman tree structure for building ---- */
struct htree    {
    BYTECOUNTER cnt;        /* character frequency         */
    s16 parent;             /* offset to parent node       */
    s16 right;              /* offset to right child node  */
    s16 left;               /* offset to left child node   */
};

/* ---- Huffman tree structure in compressed file ---- */
struct htr    {
    s16 right;              /* offset to right child node  */
    s16 left;               /* offset to left child node   */
};

extern struct htr *HelpTree;

void buildtree(void);
FILE *OpenHelpFile(const char *fn, const char *md);
void HelpFilePosition(u32 *, u16 *);
void *GetHelpLine(char *c);
void SeekHelpLine(u32, u16);

#endif


