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
#ifndef  COCO_SYNTAX_H
#define  COCO_SYNTAX_H

#ifndef  COCO_BITARRAY_H
#include  "BitArray.h"
#endif

#ifndef  COCO_ARRAYLIST_H
#include  "ArrayList.h"
#endif

struct CcSyntax_s {
    CcGlobals_t * globals;
    CcArrayList_t symSet;
    CcArrayList_t nodes;

    CcSymbol_t * gramSy;
    CcSymbol_t * eofSy;
    CcSymbol_t * noSym;
    CcSymbol_t * curSy;
    CcBitArray_t * visited;
    CcBitArray_t visitedSpace;
    CcBitArray_t * allSyncSets;
    CcBitArray_t allSyncSetsSpace;
};

CcSyntax_t * CcSyntax(CcSyntax_t * self, CcGlobals_t * globals);
void CcSyntax_Destruct(CcSyntax_t * self);

CcNode_t * CcSyntax_NewNodeEPS(CcSyntax_t * self);
CcNode_t * CcSyntax_NewNodeANY(CcSyntax_t * self);
CcNode_t * CcSyntax_NewNodeSEM(CcSyntax_t * self);
CcNode_t * CcSyntax_NewNodeSYNC(CcSyntax_t * self);
CcNode_t * CcSyntax_NewNodeWT(CcSyntax_t * self, CcSymbol_t * sym, int line);
CcNode_t * CcSyntax_NewNodeT(CcSyntax_t * self, CcSymbol_t * sym, int line);
CcNode_t * CcSyntax_NewNodePR(CcSyntax_t * self, CcSymbol_t * sym, int line);
CcNode_t * CcSyntax_NewNodeNT(CcSyntax_t * self, CcSymbol_t * sym, int line);
CcNode_t * CcSyntax_NewNodeRSLV(CcSyntax_t * self, CcSymbol_t * sym, int line);

void CcSyntax_MakeFirstAlt(CcSyntax_t * self, CcGraph_t * g);
void CcSyntax_MakeAlternative(CcSyntax_t * self, CcGraph_t * g1, CcGraph_t * g2);
void CcSyntax_MakeIteration(CcSyntax_t * self, CcGraph_t * g);
void CcSyntax_MakeOption(CcSyntax_t * self, CcGraph_t * g);

void CcSyntax_DeleteNodes(CcSyntax_t * self);

void CcSyntax_SetupAnys(CcSyntax_t * self);
void CcSyntax_RenumberPragmas(CcSyntax_t * self);
void CcSyntax_CompSymbolSets(CcSyntax_t * self);

#endif  /* COCO_SYNTAX_H */