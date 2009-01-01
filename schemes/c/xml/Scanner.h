/*---- license ----*/
/*-------------------------------------------------------------------------
 Coco.ATG -- Attributed Grammar
 Compiler Generator Coco/R,
 Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
 extended by M. Loeberbauer & A. Woess, Univ. of Linz
 with improvements by Pat Terry, Rhodes University.
 ported to C by Charles Wang <charlesw123456@gmail.com>

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
#ifndef COCO_CcsXmlScanner_H
#define COCO_CcsXmlScanner_H

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

/*---- defines ----*/
#define CcsXmlScanner_MAX_KEYWORD_LEN 23
#define CcsXmlScanner_CASE_SENSITIVE
#define CcsXmlParser_KEYWORD_USED
/*---- enable ----*/

typedef struct CcsXmlScanner_s CcsXmlScanner_t;
struct CcsXmlScanner_s {
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
#ifdef CcsXmlScanner_INDENTATION
    CcsBool_t      lineStart;
    int          * indent;
    int          * indentUsed;
    int          * indentLast;
#endif
};

CcsXmlScanner_t *
CcsXmlScanner(CcsXmlScanner_t * self, CcsErrorPool_t * errpool,
	   const char * filename);
void CcsXmlScanner_Destruct(CcsXmlScanner_t * self);
CcsToken_t * CcsXmlScanner_GetDummy(CcsXmlScanner_t * self);
CcsToken_t * CcsXmlScanner_Scan(CcsXmlScanner_t * self);
CcsToken_t * CcsXmlScanner_Peek(CcsXmlScanner_t * self);
void CcsXmlScanner_ResetPeek(CcsXmlScanner_t * self);
void CcsXmlScanner_IncRef(CcsXmlScanner_t * self, CcsToken_t * token);
void CcsXmlScanner_DecRef(CcsXmlScanner_t * self, CcsToken_t * token);

CcsPosition_t *
CcsXmlScanner_GetPosition(CcsXmlScanner_t * self, const CcsToken_t * begin,
		       const CcsToken_t * end);
CcsPosition_t *
CcsXmlScanner_GetPositionBetween(CcsXmlScanner_t * self, const CcsToken_t * begin,
			      const CcsToken_t * end);

EXTC_END

#endif  /* COCO_CcsXmlScanner_H */
