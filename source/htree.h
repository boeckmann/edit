/* ------------------- htree.h -------------------- */

#ifndef HTREE_H
#define HTREE_H

#include <stdio.h>
#include <stdint.h>

typedef uint16_t BYTECOUNTER;

/* ---- Huffman tree structure for building ---- */
struct htree    {
    BYTECOUNTER cnt;        /* character frequency         */
    int16_t parent;             /* offset to parent node       */
    int16_t right;              /* offset to right child node  */
    int16_t left;               /* offset to left child node   */
};

/* ---- Huffman tree structure in compressed file ---- */
struct htr    {
    int16_t right;              /* offset to right child node  */
    int16_t left;               /* offset to left child node   */
};

extern struct htr *HelpTree;

void buildtree(void);
FILE *OpenHelpFile(const char *fn, const char *md);
void HelpFilePosition(uint32_t *, uint16_t *);
void *GetHelpLine(char *);
void SeekHelpLine(uint32_t, uint16_t);

#endif


