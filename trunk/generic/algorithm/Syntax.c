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
#include  "Syntax.h"
#include  "Globals.h"
#include  "syntax/Nodes.h"

CcSyntax_t *
CcSyntax(CcSyntax_t * self, CcGlobals_t * globals)
{
    self->globals = globals;
    CcArrayList(&self->nodes);
    CcArrayList(&self->symSet);
    return self;
}

void
CcSyntax_Destruct(CcSyntax_t * self)
{
    CcArrayList_Destruct(&self->symSet);
    CcArrayList_Destruct(&self->nodes);
}

CcNode_t *
CcSyntax_NewNodeEPS(CcSyntax_t * self)
{
}

CcNode_t *
CcSyntax_NewNodeANY(CcSyntax_t * self)
{
}

CcNode_t *
CcSyntax_NewNodeSEM(CcSyntax_t * self)
{
}

CcNode_t *
CcSyntax_NewNodeSYNC(CcSyntax_t * self)
{
}

CcNode_t *
CcSyntax_NewNodeWT(CcSyntax_t * self, CcSymbol_t * sym, int line)
{
}

CcNode_t *
CcSyntax_NewNodeT(CcSyntax_t * self, CcSymbol_t * sym, int line)
{
}

CcNode_t *
CcSyntax_NewNodePR(CcSyntax_t * self, CcSymbol_t * sym, int line)
{
}

CcNode_t *
CcSyntax_NewNodeNT(CcSyntax_t * self, CcSymbol_t * sym, int line)
{
}

CcNode_t *
CcSyntax_NewNodeRSLV(CcSyntax_t * self, CcSymbol_t * sym, int line)
{
}

void
CcSyntax_MakeFirstAlt(CcSyntax_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeFirstAlt(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcSyntax_MakeAlternative(CcSyntax_t * self, CcGraph_t * g1, CcGraph_t * g2)
{
    CcNode_t * p = CcGraph_MakeAlternative(g1, g2, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcSyntax_MakeIteration(CcSyntax_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeIteration(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcSyntax_MakeOption(CcSyntax_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeOption(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcSyntax_DeleteNodes(CcSyntax_t * self)
{
    CcArrayList_Clear(&self->nodes);
}

static CcsBool_t CcSyntax_DelGraph(CcSyntax_t * self, CcNode_t * p);
static CcsBool_t CcSyntax_DelSubGraph(CcSyntax_t * self, CcNode_t * p);
static CcsBool_t CcSyntax_DelNode(CcSyntax_t * self, CcNode_t * p);

static CcsBool_t
CcSyntax_DelGraph(CcSyntax_t * self, CcNode_t * p)

{
    return p == NULL ||
	(CcSyntax_DelNode(self, p) && CcSyntax_DelGraph(self, p->next));
}

static CcsBool_t
CcSyntax_DelSubGraph(CcSyntax_t * self, CcNode_t * p)
{
    return p == NULL ||
	(CcSyntax_DelNode(self, p) && CcSyntax_DelSubGraph(self, p->next));
}

static CcsBool_t
CcSyntax_DelNode(CcSyntax_t * self, CcNode_t * p)
{
    const CcNodeType_t * ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_nt) {
	CcNodeNT_t * p0 = (CcNodeNT_t *)p;
	CcSymbolNT_t * sym = (CcSymbolNT_t *)p0->sym;
	return sym->deletable;
    } else if (ptype == node_alt) {
	return CcSyntax_DelSubGraph(self, p->sub) ||
	    (p->down != NULL && CcSyntax_DelSubGraph(self, p->down));
    }
    return ptype == node_iter || ptype == node_opt || ptype == node_sem ||
	ptype == node_eps || ptype == node_rslv || ptype == node_sync;
}

static void
CcSyntax_First0(CcSyntax_t * self, CcBitArray_t * ret,
		CcNode_t * p, CcBitArray_t * mark)
{
    CcBitArray_t fs, fs0;
    const CcNodeType_t * type;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    CcBitArray(&fs, symtab->terminals.Count);
    while (p != NULL && !CcBitArray_Get(mark, p->n)) {
	CcBitArray_Set(mark, p->n, TRUE);
	type = (const CcNodeType_t *)(((CcObject_t *)p)->type);
	if (type == node_nt) {
	    CcNodeNT_t * p0 = (CcNodeNT_t *)p;
	    CcSymbolNT_t * sym = (CcSymbolNT_t *)p0->sym;
	    if (sym->firstReady) {
		CcBitArray_Or(&fs, &sym->first);
	    } else {
		CcSyntax_First0(self, &fs0, sym->graph, mark);
		CcBitArray_Or(&fs, &fs0);
		CcBitArray_Destruct(&fs0);
	    }
	} else if (type == node_t) {
	    CcNodeT_t * p0 = (CcNodeT_t *)p;
	    CcBitArray_Set(&fs, p0->sym->n, TRUE);
	} else if (type == node_wt) {
	    CcNodeWT_t * p0 = (CcNodeWT_t *)p;
	    CcBitArray_Set(&fs, p0->sym->n, TRUE);
	} else if (type == node_any) {
	    CcNodeANY_t * p0 = (CcNodeANY_t *)p;
	    CcBitArray_Or(&fs, &p0->set);
	} else if (type == node_alt) {
	    CcSyntax_First0(self, &fs0, p->sub, mark);
	    CcBitArray_Or(&fs, &fs0);
	    CcBitArray_Destruct(&fs0);
	    CcSyntax_First0(self, &fs0, p->down, mark);
	    CcBitArray_Or(&fs, &fs0);
	    CcBitArray_Destruct(&fs0);
	} else if (type == node_iter && type == node_opt) {
	    CcSyntax_First0(self, &fs0, p->sub, mark);
	    CcBitArray_Or(&fs, &fs0);
	    CcBitArray_Destruct(&fs0);
	}
	if (!CcSyntax_DelNode(self, p)) break;
	p = p ->next;
    }
}

static void
CcSyntax_First(CcSyntax_t * self, CcBitArray_t * ret, CcNode_t * p)
{
    CcBitArray_t fs, fs0;

    CcBitArray(&fs0, self->nodes.Count);
    CcSyntax_First0(self, &fs, p, &fs0);
    CcBitArray_Destruct(&fs0);
    CcBitArray_Destruct(ret);
    memcpy(ret, &fs, sizeof(fs));
}

static void
CcSyntax_CompFirstSets(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	CcBitArray(&sym->first, symtab->terminals.Count);
	sym->firstReady = FALSE;
    }
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	CcSyntax_First(self, &sym->first, sym->graph);
	sym->firstReady = TRUE;
    }
}

static void
CcSyntax_CompFollow(CcSyntax_t * self, CcNode_t * p)
{
    CcBitArray_t s;
    CcSymbolNT_t * sym;
    const CcNodeType_t * ptype;

    while (p != NULL && !CcBitArray_Get(&self->visited, p->n)) {
	CcBitArray_Set(&self->visited, p->n, TRUE);
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_nt) {
	    CcSyntax_First(self, &s, p->next);
	    sym = (CcSymbolNT_t *)((CcNodeNT_t *)p)->sym;
	    CcBitArray_Or(&sym->follow, &s);
	    CcBitArray_Destruct(&s);
	    if (CcSyntax_DelGraph(self, p->next))
		CcBitArray_Set(&sym->nts, self->curSy->n, TRUE);
	} else if (ptype == node_opt || ptype == node_iter) {
	    CcSyntax_CompFollow(self, p->sub);
	} else if (ptype == node_alt) {
	    CcSyntax_CompFollow(self, p->sub);
	    CcSyntax_CompFollow(self, p->down);
	}
	p = p->next;
    }
}

static void
CcSyntax_Complete(CcSyntax_t * self, CcSymbolNT_t * sym)
{
    int idx; CcSymbolNT_t * s;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    if (CcBitArray_Get(&self->visited, sym->base.n)) return;
    CcBitArray_Set(&self->visited, sym->base.n, TRUE);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	s = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (CcBitArray_Get(&sym->nts, s->base.n)) {
	    CcSyntax_Complete(self, s);
	    CcBitArray_Or(&sym->follow, &s->follow);
	    if ((CcSymbol_t *)sym == self->curSy)
		CcBitArray_Set(&sym->nts, s->base.n, FALSE);
	}
    }
}

static void
CcSyntax_CompFollowSets(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	CcBitArray(&sym->follow, symtab->terminals.Count);
	CcBitArray(&sym->nts, symtab->nonterminals.Count);
    }
    CcBitArray_Set(&((CcSymbolNT_t *)self->gramSy)->follow,
		   self->eofSy->n, TRUE);
    CcBitArray(&self->visited, self->nodes.Count);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CompFollow(self, sym->graph);
    }
    CcBitArray_Destruct(&self->visited);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	CcBitArray(&self->visited, symtab->nonterminals.Count);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_Complete(self, sym);
    }
}

static CcNode_t *
CcSyntax_LeadingAny(CcSyntax_t * self, CcNode_t * p)
{
    CcNode_t * a;
    const CcNodeType_t * ptype;

    if (p == NULL) return NULL;
    a = NULL;
    ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_any) {
	a = p;
    } else if (ptype == node_alt) {
	a = CcSyntax_LeadingAny(self, p->sub);
	if (a == NULL) a = CcSyntax_LeadingAny(self, p->down);
    } else if (ptype == node_opt || ptype == node_iter) {
	a = CcSyntax_LeadingAny(self, p->sub);
    } else if (CcSyntax_DelNode(self, p) && !p->up) {
	a = CcSyntax_LeadingAny(self, p->next);
    }
    return a;
}

static void
CcSyntax_FindAS(CcSyntax_t * self, CcNode_t * p)
{
    CcNode_t * a, * q;
    CcBitArray_t s0, s1;
    const CcNodeType_t * ptype;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_opt || ptype == node_iter) {
	    CcSyntax_FindAS(self, p->sub);
	    a = CcSyntax_LeadingAny(self, p->sub);
	    if (a != NULL) {
		CcSyntax_First(self, &s0, p->next);
		CcBitArray_Subtract(&((CcNodeANY_t *)a)->set, &s0);
		CcBitArray_Destruct(&s0);
	    }
	} else if (ptype == node_alt) {
	    CcBitArray(&s1, symtab->terminals.Count);
	    q = p;
	    while (q != NULL) {
		CcSyntax_FindAS(self, q->sub);
		a = CcSyntax_LeadingAny(self, q->sub);
		if (a != NULL) {
		    CcSyntax_First(self, &s0, p->down);
		    CcBitArray_Or(&s0, &s1);
		    CcBitArray_Subtract(&((CcNodeANY_t *)a)->set, &s0);
		    CcBitArray_Destruct(&s0);
		} else {
		    CcSyntax_First(self, &s0, q->sub);
		    CcBitArray_Or(&s1, &s0);
		}
		q = q->down;
	    }
	}
	if (p->up) break;
	p = p->next;
    }
}

static void
CcSyntax_CompAnySets(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	CcSyntax_FindAS(self, sym->graph);
    }
}

static void
CcSyntax_Expected(CcSyntax_t * self, CcBitArray_t * ret,
		  CcNode_t * p, CcSymbol_t * curSy)
{
    CcSyntax_First(self, ret, p);
    if (CcSyntax_DelGraph(self, p))
	CcBitArray_Or(ret, &((CcSymbolNT_t *)curSy)->follow);
}

static void
CcSyntax_Expected0(CcSyntax_t * self, CcBitArray_t * ret,
		   CcNode_t * p, CcSymbol_t * curSy)
{
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    const CcNodeType_t * ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_rslv) CcBitArray(ret, symtab->terminals.Count);
    else CcSyntax_Expected(self, ret, p, curSy);
}

static void
CcSyntax_CompSync(CcSyntax_t * self, CcNode_t * p)
{
    CcBitArray_t s;
    const CcNodeType_t * ptype;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    
    while (p != NULL && !CcBitArray_Get(&self->visited, p->n)) {
	ptype = (const CcNodeType_t *)p->base.type;
	CcBitArray_Set(&self->visited, p->n, TRUE);
	if (ptype == node_sync) {
	    CcSyntax_Expected(self, &s, p->next, self->curSy);
	    CcBitArray_Set(&s, self->eofSy->n, TRUE);
	    CcBitArray_Or(&self->allSyncSets, &s);
	    memcpy(&((CcNodeSYNC_t *)p)->set, &s, sizeof(s));
	} else if (ptype == node_alt) {
	    CcSyntax_CompSync(self, p->sub);
	    CcSyntax_CompSync(self, p->down);
	} else if (ptype == node_opt || ptype == node_iter) {
	    CcSyntax_CompSync(self, p->sub);
	}
	p = p->next;
    }
}

static void
CcSyntax_CompSyncSets(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    CcBitArray(&self->allSyncSets, symtab->terminals.Count);
    CcBitArray_Set(&self->allSyncSets, self->eofSy->n, TRUE);
    CcBitArray(&self->visited, self->nodes.Count);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CompSync(self, ((CcSymbolNT_t *)self->curSy)->graph);
    }
}

void
CcSyntax_RenumberPragmas(CcSyntax_t * self)
{
    CcSymbolPR_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    int idx, n = symtab->terminals.Count;
    for (idx = 0; idx < symtab->pragmas.Count; ++idx) {
	sym = (CcSymbolPR_t *)CcArrayList_Get(&symtab->pragmas, idx);
	sym->base.n = n++;
    }
}

void
CcSyntax_SetupAnys(CcSyntax_t * self)
{
    int idx; CcNode_t * p;
    const CcNodeType_t * ptype;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    for (idx = 0; idx < self->nodes.Count; ++idx) {
	p = (CcNode_t *)CcArrayList_Get(&self->nodes, idx);
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_any) {
	    CcBitArray1(&((CcNodeANY_t *)p)->set, symtab->terminals.Count);
	    CcBitArray_Set(&((CcNodeANY_t *)p)->set, self->eofSy->n, FALSE);
	}
    }
}

static void
CcSyntax_CompDeletableSymbols(CcSyntax_t * self)
{
    CcsBool_t changed;
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    do {
	changed = FALSE;
	for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	    sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	    if (!sym->deletable && sym->graph != NULL &&
		CcSyntax_DelGraph(self, sym->graph)) {
		sym->deletable = TRUE; changed = TRUE;
	    }
	}
    } while (changed);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (sym->deletable) 
	    CcsGlobals_Warning(&self->globals->base, NULL,
			       " %s deletable", sym->base.name);
    }
}

void
CcSyntax_CompSymbolSets(CcSyntax_t * self)
{
    CcSyntax_CompDeletableSymbols(self);
    CcSyntax_CompFirstSets(self);
    CcSyntax_CompFollowSets(self);
    CcSyntax_CompAnySets(self);
    CcSyntax_CompSyncSets(self);
}

typedef struct {
    CcSymbol_t * left;
    CcSymbol_t * right;
} CNode_t;

CNode_t *
CNode(CcSymbol_t * l, CcSymbol_t * r)
{
    CNode_t * self = CcMalloc(sizeof(CNode_t));
    self->left = l; self->right =r;
    return self;
}

static void
CcSyntax_GetSingles(CcSyntax_t * self, CcNode_t * p, CcArrayList_t * singles)
{
    const CcNodeType_t * ptype;
    if (p == NULL) return; /* end of graph */
    ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_nt) {
	if (p->up || CcSyntax_DelGraph(self, p->next))
	    CcArrayList_Add(singles, (CcObject_t *)((CcNodeNT_t *)p)->sym);
    } else if (ptype == node_alt || ptype == node_iter || ptype == node_opt) {
	if (p->up || CcSyntax_DelGraph(self, p->next)) {
	    CcSyntax_GetSingles(self, p->sub, singles);
	    if (ptype == node_alt) CcSyntax_GetSingles(self, p->down, singles);
	}
    }
    if (p->up && CcSyntax_DelNode(self, p))
	CcSyntax_GetSingles(self, p->next, singles);
}

static CcsBool_t
CcSyntax_NoCircularProductions(CcSyntax_t * self)
{
    int idx, idxj; CcSymbolNT_t * sym; CcSymbol_t * s; CNode_t * m, * n;
    CcsBool_t ok, changed, onLeftSide, onRightSide;
    CcArrayList_t list, singles;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    CcArrayList(&list);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	CcArrayList(&singles);
	/* Get nonterminals s such that sym-->s */
	CcSyntax_GetSingles(self, sym->graph, &singles);
	for (idxj = 0; idxj < singles.Count; ++idxj) {
	    s = (CcSymbol_t *)CcArrayList_Get(&singles, idxj);
	    n = CNode((CcSymbol_t *)sym, s);
	    CcArrayList_Add(&list, (CcObject_t *)n);
	}
	CcArrayList_Destruct(&singles);
    }
    do {
	changed = FALSE;
	for (idx = 0; idx < list.Count; ++idx) {
	    n = (CNode_t *)CcArrayList_Get(&list, idx);
	    onLeftSide = FALSE; onRightSide = FALSE;
	    for (idxj = 0; idxj < list.Count; ++idxj) {
		if (m->left == m->right) onRightSide = TRUE;
		if (m->right == m->left) onLeftSide = TRUE;
	    }
	    if (!onLeftSide || !onRightSide) {
		CcArrayList_Remove(&list, (CcObject_t *)n); --idx; changed = TRUE;
	    }
	}
    } while (changed);
    ok = TRUE;
    for (idx = 0; idx < list.Count; ++idx) {
	ok = FALSE;
	n = (CNode_t *)CcArrayList_Get(&list, idx);
	CcsGlobals_SemErr(&self->globals->base, NULL, " %s --> %s",
			  n->left->name, n->right->name);
    }
    CcArrayList_Destruct(&list);
    return ok;
}

static void
CcSyntax_LL1Error(CcSyntax_t * self, int cond, CcSymbol_t * sym)
{
    const char * s0, * s1, * s2;
    if (sym) { s0 = sym->name; s1 = " is "; }
    else { s0 = s1 = ""; }
    switch (cond) {
    case 1: s2 = "start of several alternatives"; break;
    case 2: s2 = "start & successor of deletable structure"; break;
    case 3: s2 = "an ANY node that matches no symbol"; break;
    case 4: s2 = "contents of [...] or {...} must not be deletable"; break;
    default: s2 = ""; break;
    }
    CcsGlobals_Warning(&self->globals->base, NULL,
		       "  LL1 warning in %s: %s%s%s", self->curSy->name, s0, s1, s2);
}

static void
CcSyntax_CheckOverlap(CcSyntax_t * self, CcBitArray_t * s1, CcBitArray_t * s2, int cond)
{
    int idx; CcSymbolT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    for (idx = 0; idx < symtab->terminals.Count; ++idx) {
	sym = (CcSymbolT_t *)CcArrayList_Get(&symtab->terminals, idx);
	if (CcBitArray_Get(s1, sym->base.n) && CcBitArray_Get(s2, sym->base.n))
	    CcSyntax_LL1Error(self, cond, (CcSymbol_t *)sym);
    }
}

/* FIX ME */
static void
CcSyntax_CheckAlts(CcSyntax_t * self, CcNode_t * p)
{
    CcBitArray_t s1, s2; CcNode_t * q;
    const CcNodeType_t * ptype;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_alt) {
	    q = p;
	    CcBitArray(&s1, symtab->terminals.Count);
	    while (q != NULL) { /* For all alternatives */
		CcSyntax_Expected0(self, &s2, q->sub, self->curSy);
		CcSyntax_CheckOverlap(self, &s1, &s2, 1);
		CcBitArray_Or(&s1, &s2);
		CcSyntax_CheckAlts(self, q->sub);
		q = q->down;
		CcBitArray_Destruct(&s2);
	    }
	    CcBitArray_Destruct(&s1);
	} else if (ptype == node_opt || ptype == node_iter) {
	    /* E.g. [[...]] */
	    if (CcSyntax_DelSubGraph(self, p->sub)) {
		CcSyntax_LL1Error(self, 4, NULL);
	    } else {
		CcSyntax_Expected0(self, &s1, p->sub, self->curSy);
		CcSyntax_Expected(self, &s2, p->next, self->curSy);
		CcSyntax_CheckOverlap(self, &s1, &s2, 2);
		CcSyntax_CheckAlts(self, p->sub);
		CcBitArray_Destruct(&s1);
		CcBitArray_Destruct(&s2);
	    }
	} else if (ptype == node_any) {
	    /* E.g. {ANY} ANY or [ANY] ANY */
	    if (CcBitArray_Elements(&((CcNodeANY_t *)p)->set) == 0)
		CcSyntax_LL1Error(self, 3, NULL);
	}
	if (p->up) break;
	p = p->next;
    }
}

static void
CcSyntax_CheckLL1(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CheckAlts(self, sym->graph);
    }
}

/*------------- check if resolvers are legal  --------------------*/
static void
CcSyntax_CheckRes(CcSyntax_t * self, CcNode_t * p, CcsBool_t rslvAllowed)
{
    CcNode_t * q;
    CcBitArray_t expected, soFar, fs, fsNext;
    const CcNodeType_t * ptype, * psubtype;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_alt) {
	    CcBitArray(&expected, symtab->terminals.Count);
	    for (q = p; q != NULL; q = q->down) {
		CcSyntax_Expected0(self, &fs, q->sub, self->curSy);
		CcBitArray_Or(&expected, &fs);
		CcBitArray_Destruct(&fs);
	    }
	    CcBitArray(&soFar, symtab->terminals.Count);
	    for (q = p; q != NULL; q = q->down) {
		psubtype = (const CcNodeType_t *)q->sub->base.type;
		if (psubtype == node_rslv) {
		    CcSyntax_Expected(self, &fs, q->sub->next, self->curSy);
		    if (CcBitArray_Intersect(&fs, &soFar))
			CcsGlobals_Warning(&self->globals->base, NULL,
					   "Resolver will never be evaluated. "
					   "Place it at previous conflicting alternative.");
		    if (!CcBitArray_Intersect(&fs, &expected))
			CcsGlobals_Warning(&self->globals->base, NULL,
					   "Misplaced resolver: no LL(1) conflict.");
		    CcBitArray_Destruct(&fs);
		} else {
		    CcSyntax_Expected(self, &fs, q->sub, self->curSy);
		    CcBitArray_Or(&soFar, &fs);
		}
		CcSyntax_CheckRes(self, q->sub, TRUE);
	    }
	} else if (ptype == node_iter || ptype == node_opt) {
	    psubtype = (const CcNodeType_t *)p->sub->base.type;
	    if (psubtype == node_rslv) {
		CcSyntax_First(self, &fs, p->sub->next);
		CcSyntax_Expected(self, &fsNext, p->next, self->curSy);
		if (!CcBitArray_Intersect(&fs, &fsNext))
		    CcsGlobals_Warning(&self->globals->base, NULL,
				       "Misplaced resolver: no LL(1) conflict.");
	    }
	    CcSyntax_CheckRes(self, p->sub, TRUE);
	} else if (ptype == node_rslv) {
	    if (!rslvAllowed)
		CcsGlobals_Warning(&self->globals->base, NULL,
				   "Misplaced resolver: no alternative.");
	}
	if (p->up) break;
	p = p->next;
	rslvAllowed = FALSE;
    }
}

static void
CcSyntax_CheckResolvers(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CheckRes(self, sym->graph, FALSE);
    }
}

/*------------- check if every nts has a production --------------------*/
static CcsBool_t
CcSyntax_NtsComplete(CcSyntax_t * self)
{
    int idx; CcSymbolNT_t * sym;
    CcsBool_t complete = TRUE;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (sym->graph == NULL) {
	    complete = FALSE;
	    CcsGlobals_SemErr(&self->globals->base, NULL,
			      "No production for %s", sym->base.name);
	}
    }
    return complete;
}

static void
CcSyntax_MarkReachedNts(CcSyntax_t * self, CcNode_t * p)
{
    const CcNodeType_t * ptype;
    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_nt &&
	    !CcBitArray_Get(&self->visited, ((CcNodeNT_t *)p)->sym->n)) {
	    /* new nt reached */
	    CcBitArray_Set(&self->visited, ((CcNodeNT_t *)p)->sym->n, TRUE);
	    CcSyntax_MarkReachedNts(self, ((CcSymbolNT_t *)((CcNodeNT_t *)p)->sym)->graph);
	} else if (ptype == node_alt || ptype == node_iter || ptype == node_opt) {
	    CcSyntax_MarkReachedNts(self, p->sub);
	    if (ptype == node_alt) CcSyntax_MarkReachedNts(self, p->down);
	}
	if (p->up) break;
	p = p->next;
    }
}

static CcsBool_t
CcSyntax_AllNtReached(CcSyntax_t * self)
{
    int idx; CcSymbol_t * sym;
    CcsBool_t ok = TRUE;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;

    CcBitArray(&self->visited, symtab->nonterminals.Count);
    CcBitArray_Set(&self->visited, self->gramSy->n, TRUE);
    CcSyntax_MarkReachedNts(self, ((CcSymbolNT_t *)self->gramSy)->graph);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbol_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (!CcBitArray_Get(&self->visited, sym->n)) {
	    ok = FALSE;
	    CcsGlobals_SemErr(&self->globals->base, NULL,
			      " %s cannot be reached", sym->name);
	}
    }
    return ok;
}

/*--------- check if every nts can be derived to terminals  ------------ */
/* true if graph can be derived to terminals */
CcsBool_t
CcSyntax_IsTerm(CcSyntax_t *self, CcNode_t * p, CcBitArray_t * mark) {
    const CcNodeType_t * ptype;

    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_nt &&
	    !CcBitArray_Get(mark, (((CcNodeNT_t *)p)->sym)->n))
	    return FALSE;
	if (ptype == node_alt && !CcSyntax_IsTerm(self, p->sub, mark) &&
	    (p->down == NULL || !CcSyntax_IsTerm(self, p->down, mark)))
	    return FALSE;
	if (p->up) break;
	p = p->next;
    }
    return TRUE;
}

static CcsBool_t
CcSyntax_AllNtToTerm(CcSyntax_t * self)
{
    CcBitArray_t mark;
    int idx; CcSymbolNT_t * sym;
    CcsBool_t changed, ok = TRUE;
    CcSymbolTable_t * symtab = &self->globals->symbolTab;
    
    CcBitArray(&mark, symtab->nonterminals.Count);
    do {
	changed = FALSE;
	for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	    sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	    if (!CcBitArray_Get(&mark, sym->base.n) &&
		CcSyntax_IsTerm(self, sym->graph, &mark)) {
		CcBitArray_Set(&mark, sym->base.n, TRUE);
		changed = TRUE;
	    }
	}
    } while (changed);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (!CcBitArray_Get(&mark, sym->base.n)) {
	    ok = FALSE;
	    CcsGlobals_SemErr(&self->globals->base, NULL,
			      " %s cannot be derived to terminals", sym->base.name);
	}
    }
    return ok;
}

CcsBool_t
CcSyntax_GrammarOk(CcSyntax_t * self)
{
    CcsBool_t ok = CcSyntax_NtsComplete(self) &&
	CcSyntax_AllNtReached(self) &&
	CcSyntax_NoCircularProductions(self) &&
	CcSyntax_AllNtToTerm(self);
    if (ok) {
	CcSyntax_CheckResolvers(self);
	CcSyntax_CheckLL1(self);
    }
    return ok;
}
