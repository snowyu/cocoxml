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
#ifndef  COCO_LEXICAL_H
#define  COCO_LEXICAL_H

#ifndef  COCO_ARRAYLIST_H
#include "ArrayList.h"
#endif

#ifndef  COCO_HASHTABLE_H
#include "HashTable.h"
#endif

#ifndef  COCO_EBNF_H
#include "EBNF.h"
#endif

EXTC_BEGIN

struct CcLexical_s {
    CcEBNF_t        base;
    CcGlobals_t   * globals;

    CcCharSet_t   * ignored;
    CcsBool_t       ignoreCase;
    CcsBool_t       indentUsed;
    int             indentIn;
    int             indentOut;
    int             indentErr;
    CcArrayList_t   states;
    CcArrayList_t   classes;
    CcHashTable_t   literals;
    CcMelted_t    * firstMelted;
    CcComment_t   * firstComment;

    int             lastSimState;
    int             maxStates;

    CcSymbol_t    * curSy;
    CcsBool_t       dirtyLexical;
    CcsBool_t       hasCtxMoves;
};

CcLexical_t * CcLexical(CcLexical_t * self, CcGlobals_t * globals);
void CcLexical_Destruct(CcLexical_t * self);

CcGraph_t *
CcLexical_StrToGraph(CcLexical_t * self, const char * str, const CcsToken_t * t);
void CcLexical_SetContextTrans(CcLexical_t * self, CcNode_t * p);

CcCharClass_t *
CcLexical_NewCharClass(CcLexical_t * self, const char * name, CcCharSet_t * s);

CcCharClass_t *
CcLexical_FindCharClassN(CcLexical_t * self, const char * name);

CcCharClass_t *
CcLexical_FindCharClassC(CcLexical_t * self, const CcCharSet_t * s);

CcCharSet_t * CcLexical_CharClassSet(CcLexical_t * self, int idx);

void CcLexical_ConvertToStates(CcLexical_t * self, CcNode_t * p, CcSymbol_t * sym);
void CcLexical_MatchLiteral(CcLexical_t * self, const CcsToken_t * t,
			    const char * s, CcSymbol_t * sym);
void CcLexical_MakeDeterministic(CcLexical_t * self);

void
CcLexical_NewComment(CcLexical_t * self, const CcsToken_t * token,
		     CcNode_t * from, CcNode_t * to, CcsBool_t nested);

CcsBool_t CcLexical_Finish(CcLexical_t * self);

typedef struct {
    int keyFrom;
    int keyTo;
    int state;
}  CcLexical_StartTab_t;

CcLexical_StartTab_t *
CcLexical_GetStartTab(const CcLexical_t * self, int * retNumEle);

typedef struct {
    char * name;
    int index;
}  CcLexical_Identifier_t;

CcLexical_Identifier_t *
CcLexical_GetIdentifiers(const CcLexical_t * self, int * retNumEle);

void CcLexical_Identifiers_Destruct(CcLexical_Identifier_t * self, int numEle);

void CcLexical_TargetStates(const CcLexical_t * self, CcBitArray_t * mask);

#ifdef  NDEBUG
#define  CcLexical_DumpStates(self)   ((void)0)
#else
void CcLexical_DumpStates(const CcLexical_t * self);
#endif

EXTC_END

#endif  /* COCO_LEXICAL_H */
