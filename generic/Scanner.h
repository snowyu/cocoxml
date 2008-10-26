/* -*- c -*- */
/*---- open(Scanner.h) S ----*/
/*---- open(c.Scanner.frame) F ----*/
/*-------------------------------------------------------------------------
  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the 
  Free Software Foundation; either version 2, or (at your option) any 
  later version.

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
  for more details.

  You should have received a copy of the GNU General Public License along 
  with this program; if not, write to the Free Software Foundation, Inc., 
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  As an exception, it is allowed to write an extension of Coco/R that is
  used as a plugin in non-free software.

  If not otherwise stated, any source code generated by Coco/R (other than 
  Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
/*---- enable ----*/
#ifndef  COCO_SCANNER_H
#define  COCO_SCANNER_H

#include  <stdio.h>

#ifdef __cplusplus
#define  EXTC_BEGIN extern "C" {
#define  EXTC_END   }
#else
#define  EXTC_BEGIN
#define  EXTC_END
#endif

EXTC_BEGIN

#define COCO_WCHAR_MAX 65535
#define EoF            -1
#define ErrorChr       -2

typedef struct Token_s Token_t;
struct Token_s
{
    Token_t * next;
    int kind;
    int pos;
    int col;
    int line;
    char * val;
};

typedef struct {
    FILE * fp;
    long   start;
    char * buf;
    char * busyFirst; /* The first char used by Token_t. */
    char * lockCur;   /* The value of of cur when locked. */
    char * lockNext;  /* The value of next when locked. */
    char * cur;    /* The first char of the current char in Scanner_t.*/
    char * next;   /* The first char of the next char in Scanner_t. */
    char * loaded;
    char * last;
} Buffer_t;

typedef struct {
    int        eofSym;
    int        noSym;
    int        maxT;

    Token_t  * dummyToken;

    Token_t  * busyTokenList;
    Token_t ** curToken;
    Token_t ** peekToken;

    int        ch;
    int        chBytes;
    int        pos;
    int        line;
    int        col;
    int        oldEols;
    int        oldEolsEOL;

    Buffer_t   buffer;
} Scanner_t;

Scanner_t * Scanner(Scanner_t * self, const char * filename);
void Scanner_Destruct(Scanner_t * self);
const Token_t * Scanner_GetDummy(Scanner_t * self);
void Scanner_Release(Scanner_t * self, const Token_t * token);
const Token_t * Scanner_Scan(Scanner_t * self);
const Token_t * Scanner_Peek(Scanner_t * self);
void Scanner_ResetPeek(Scanner_t * self);

EXTC_END

#endif /* COCO_SCANNER_H */