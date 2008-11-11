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
#ifndef  COCO_DEFS_H
#define  COCO_DEFS_H

#ifndef  COCO_CDEFS_H
#include  "c/CDefs.h"
#endif

EXTC_BEGIN

/* Basic DataStructures */
typedef struct CcObjectType_s CcObjectType_t;
typedef struct CcObject_s CcObject_t;
typedef struct CcArrayList_s CcArrayList_t;
typedef struct CcBitArray_s CcBitArray_t;
typedef struct CcHashTable_s CcHashTable_t;

/* EBNF types */
typedef struct CcNode_s  CcNode_t;
typedef struct CcGraph_s CcGraph_t;

extern const CcObjectType_t * node_alt;
extern const CcObjectType_t * node_iter;
extern const CcObjectType_t * node_opt;

/* OutputScheme types. */
typedef struct CcOutputSchemeType_s CcOutputSchemeType_t;
typedef struct CcOutputScheme_s CcOutputScheme_t;
typedef struct CcSourceOutputSchemeType_s CcSourceOutputSchemeType_t;
typedef struct CcSourceOutputScheme_s CcSourceOutputScheme_t;

/* Algorithm types */
typedef struct CcLexical_s CcLexical_t;
typedef struct CcSyntax_s CcSyntax_t;
typedef struct CcSymbolTable_s CcSymbolTable_t;

/* Symbol */
typedef struct CcSymbol_s CcSymbol_t;
typedef struct CcSymbolT_s CcSymbolT_t;
typedef struct CcSymbolPR_s CcSymbolPR_t;
typedef struct CcSymbolNT_s CcSymbolNT_t;

typedef enum {
    symbol_fixedToken = 0,
    symbol_classToken = 1,
    symbol_litToken = 2,
    symbol_classLitToken = 3,
} CcSymbol_TokenKind_t;
extern const CcObjectType_t * symbol_t;
extern const CcObjectType_t * symbol_nt;
extern const CcObjectType_t * symbol_pr;
extern const CcObjectType_t * symbol_unknown;
extern const CcObjectType_t * symbol_rslv;

/* Lexical types */
typedef struct CcAction_s CcAction_t;
typedef struct CcCharSet_s CcCharSet_t;
typedef struct CcCharClass_s CcCharClass_t;
typedef struct CcComment_s CcComment_t;
typedef struct CcMelted_s CcMelted_t;
typedef struct CcState_s CcState_t;
typedef struct CcTarget_s CcTarget_t;


#define CcMalloc(size) _CcMalloc_(size, __FILE__, __LINE__)
void * _CcMalloc_(size_t size, const char * fname, int line);

#define CcRealloc(ptr, size) _CcRealloc_(ptr, size, __FILE__, __LINE__)
void * _CcRealloc_(void * ptr, size_t size, const char * fname, int line);

#define CcFree(ptr) _CcFree_(ptr, __FILE__, __LINE__)
void _CcFree_(void * ptr, const char * fname, int line);

#define CcStrdup(str) _CcStrdup_(str, __FILE__, __LINE__)
char * _CcStrdup_(const char * str, const char * fname, int line);

typedef struct CcGlobals_s CcGlobals_t;

EXTC_END

#endif /* COCO_DEFS_H */
