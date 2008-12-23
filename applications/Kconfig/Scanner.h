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
#ifndef COCO_SCANNER_H
#define COCO_SCANNER_H

#ifndef  COCO_TOKEN_H
#include "c/Token.h"
#endif

#ifndef  COCO_BUFFER_H
#include "c/Buffer.h"
#endif

#ifndef  COCO_POSITION_H
#include "c/Position.h"
#endif

EXTC_BEGIN

typedef struct KcScanner_s KcScanner_t;
struct KcScanner_s {
    CcsErrorPool_t * errpool;

    int            eofSym;
    int            noSym;
    int            maxT;

    CcsToken_t   * dummyToken;

    CcsToken_t   * busyTokenList;
    CcsToken_t  ** curToken;
    CcsToken_t  ** peekToken;

    int            ch;
    int            chBytes;
    int            pos;
    int            line;
    int            col;
    int            oldEols;
    int            oldEolsEOL;

    CcsBuffer_t    buffer;
};

KcScanner_t *
KcScanner(KcScanner_t * self, CcsErrorPool_t * errpool,
	   const char * filename);
void KcScanner_Destruct(KcScanner_t * self);
CcsToken_t * KcScanner_GetDummy(KcScanner_t * self);
CcsToken_t * KcScanner_Scan(KcScanner_t * self);
CcsToken_t * KcScanner_Peek(KcScanner_t * self);
void KcScanner_ResetPeek(KcScanner_t * self);
void KcScanner_IncRef(KcScanner_t * self, CcsToken_t * token);
void KcScanner_DecRef(KcScanner_t * self, CcsToken_t * token);

CcsPosition_t *
KcScanner_GetPosition(KcScanner_t * self, const CcsToken_t * begin,
		       const CcsToken_t * end);
CcsPosition_t *
KcScanner_GetPositionBetween(KcScanner_t * self, const CcsToken_t * begin,
			      const CcsToken_t * end);

EXTC_END

#endif  /* COCO_SCANNER_H */