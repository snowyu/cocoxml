/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: GPLv2 (see LICENSE-GPL)
-------------------------------------------------------------------------*/
#include  "Syntax.h"
#include  "Globals.h"
#include  "syntax/Nodes.h"
#include  "c/ErrorPool.h"

CcSyntax_t *
CcSyntax(CcSyntax_t * self, CcGlobals_t * globals)
{
    CcEBNF(&self->base);
    self->globals = globals;
    self->members = NULL;
    self->constructor = NULL;
    self->destructor = NULL;
    self->schemeName = NULL;
    self->grammarPrefix = NULL;
    self->gramSy = NULL;
    self->eofSy = CcSymbolTable_NewTerminal(&self->globals->symtab, "EOF", 0);
    self->noSy = NULL;
    self->curSy = NULL;
    self->visited = NULL;
    self->allSyncSets = NULL;
    CcArrayList(&self->errors);
    return self;
}

void
CcSyntax_Destruct(CcSyntax_t * self)
{
    CcArrayList_Destruct(&self->errors);
    if (self->allSyncSets) CcBitArray_Destruct(self->allSyncSets);
    if (self->visited) CcBitArray_Destruct(self->visited);
    if (self->grammarPrefix) CcFree(self->grammarPrefix);
    if (self->schemeName) CcFree(self->schemeName);
    if (self->destructor) CcsPosition_Destruct(self->destructor);
    if (self->constructor) CcsPosition_Destruct(self->constructor);
    if (self->members) CcsPosition_Destruct(self->members);
    CcEBNF_Destruct(&self->base);
}

CcNode_t *
CcSyntax_NodeFromSymbol(CcSyntax_t * self, const CcSymbol_t * sym, int line,
			CcsBool_t weak)
{
    if (sym->base.type == symbol_t && weak) {
	return (CcNode_t *)CcEBNF_NewNode(&self->base, CcNodeWT(line, sym));
    } else if (sym->base.type == symbol_t) {
	return (CcNode_t *)CcEBNF_NewNode(&self->base, CcNodeT(line, sym));
    } else if (sym->base.type == symbol_pr) {
	return (CcNode_t *)CcEBNF_NewNode(&self->base, CcNodePR(line));
    } else if (sym->base.type == symbol_nt) {
	return (CcNode_t *)CcEBNF_NewNode(&self->base, CcNodeNT(line, sym));
    } else if (sym->base.type == symbol_rslv) {
	return (CcNode_t *)CcEBNF_NewNode(&self->base, CcNodeRslv(line));
    }
    CcsAssert(0);
    return NULL;
}

static void
CcSyntax_First0(CcSyntax_t * self, CcBitArray_t * ret,
		CcNode_t * p, CcBitArray_t * mark)
{
    CcBitArray_t fs0;
    CcSymbolTable_t * symtab = &self->globals->symtab;

    CcBitArray(ret, symtab->terminals.Count);
    while (p != NULL && !CcBitArray_Get(mark, p->base.index)) {
	CcBitArray_Set(mark, p->base.index, TRUE);
	if (p->base.type == node_nt) {
	    CcNodeNT_t * p0 = (CcNodeNT_t *)p;
	    CcSymbolNT_t * sym = (CcSymbolNT_t *)p0->sym;
	    if (sym->firstReady) {
		CcBitArray_Or(ret, sym->first);
	    } else {
		CcSyntax_First0(self, &fs0, sym->graph, mark);
		CcBitArray_Or(ret, &fs0);
		CcBitArray_Destruct(&fs0);
	    }
	} else if (p->base.type == node_t) {
	    CcNodeT_t * p0 = (CcNodeT_t *)p;
	    CcBitArray_Set(ret, p0->sym->kind, TRUE);
	} else if (p->base.type == node_wt) {
	    CcNodeWT_t * p0 = (CcNodeWT_t *)p;
	    CcBitArray_Set(ret, p0->sym->kind, TRUE);
	} else if (p->base.type == node_any) {
	    CcNodeANY_t * p0 = (CcNodeANY_t *)p;
	    CcBitArray_Or(ret, p0->set);
	} else if (p->base.type == node_alt) {
	    CcSyntax_First0(self, &fs0, p->sub, mark);
	    CcBitArray_Or(ret, &fs0);
	    CcBitArray_Destruct(&fs0);
	    CcSyntax_First0(self, &fs0, p->down, mark);
	    CcBitArray_Or(ret, &fs0);
	    CcBitArray_Destruct(&fs0);
	} else if (p->base.type == node_iter || p->base.type == node_opt) {
	    CcSyntax_First0(self, &fs0, p->sub, mark);
	    CcBitArray_Or(ret, &fs0);
	    CcBitArray_Destruct(&fs0);
	}
	if (!CcNode_DelNode(p)) break;
	p = p->next;
    }
}

void
CcSyntax_First(CcSyntax_t * self, CcBitArray_t * ret, CcNode_t * p)
{
    CcBitArray_t fs0;

    CcBitArray(&fs0, self->base.nodes.Count);
    CcSyntax_First0(self, ret, p, &fs0);
    CcBitArray_Destruct(&fs0);
}

static void
CcSyntax_CompFirstSets(CcSyntax_t * self)
{
    CcBitArray_t fs;
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	sym->first = CcBitArray(&sym->firstSpace, symtab->terminals.Count);
	sym->firstReady = FALSE;
    }
    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	CcSyntax_First(self, &fs, sym->graph);
	CcBitArray_Destruct(sym->first);
	memcpy(sym->first, &fs, sizeof(fs));
	sym->firstReady = TRUE;
    }
}

static void
CcSyntax_CompFollow(CcSyntax_t * self, CcNode_t * p)
{
    CcBitArray_t s;
    CcSymbolNT_t * sym;

    while (p != NULL && !CcBitArray_Get(self->visited, p->base.index)) {
	CcBitArray_Set(self->visited, p->base.index, TRUE);
	if (p->base.type == node_nt) {
	    CcSyntax_First(self, &s, p->next);
	    sym = (CcSymbolNT_t *)((CcNodeNT_t *)p)->sym;
	    CcBitArray_Or(sym->follow, &s);
	    CcBitArray_Destruct(&s);
	    if (CcNode_DelGraph(p->next))
		CcBitArray_Set(sym->nts, self->curSy->kind, TRUE);
	} else if (p->base.type == node_opt || p->base.type == node_iter) {
	    CcSyntax_CompFollow(self, p->sub);
	} else if (p->base.type == node_alt) {
	    CcSyntax_CompFollow(self, p->sub);
	    CcSyntax_CompFollow(self, p->down);
	}
	p = p->next;
    }
}

static void
CcSyntax_Complete(CcSyntax_t * self, CcSymbolNT_t * sym)
{
    CcSymbolNT_t * s; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    if (CcBitArray_Get(self->visited, sym->base.kind)) return;
    CcBitArray_Set(self->visited, sym->base.kind, TRUE);
    for (s = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 s; s = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	if (CcBitArray_Get(sym->nts, s->base.kind)) {
	    CcSyntax_Complete(self, s);
	    CcBitArray_Or(sym->follow, s->follow);
	    if ((CcSymbol_t *)sym == self->curSy)
		CcBitArray_Set(sym->nts, s->base.kind, FALSE);
	}
    }
}

static void
CcSyntax_CompFollowSets(CcSyntax_t * self)
{
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * tarr = &symtab->terminals;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	sym->follow = CcBitArray(&sym->followSpace, tarr->Count);
	sym->nts = CcBitArray(&sym->ntsSpace, ntarr->Count);
    }
    CcBitArray_Set(((CcSymbolNT_t *)self->gramSy)->follow,
		   self->eofSy->kind, TRUE);

    self->visited = CcBitArray(&self->visitedSpace, self->base.nodes.Count);
    for (sym =(CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CompFollow(self, sym->graph);
    }
    CcBitArray_Destruct(self->visited); self->visited = NULL;

    for (sym =(CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	self->visited = CcBitArray(&self->visitedSpace, ntarr->Count);
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_Complete(self, sym);
	CcBitArray_Destruct(self->visited); self->visited = NULL;
    }
}

static CcNode_t *
CcSyntax_LeadingAny(CcSyntax_t * self, CcNode_t * p)
{
    CcNode_t * a;

    if (p == NULL) return NULL;
    if (p->base.type == node_any) {
	a = p;
    } else if (p->base.type == node_alt) {
	a = CcSyntax_LeadingAny(self, p->sub);
	if (a == NULL) a = CcSyntax_LeadingAny(self, p->down);
    } else if (p->base.type == node_opt || p->base.type == node_iter) {
	a = CcSyntax_LeadingAny(self, p->sub);
    } else if (CcNode_DelNode(p) && !p->up) {
	a = CcSyntax_LeadingAny(self, p->next);
    } else {
	a = NULL;
    }
    return a;
}

static void
CcSyntax_FindAS(CcSyntax_t * self, CcNode_t * p)
{
    CcNode_t * a, * q;
    CcBitArray_t s0, s1;
    CcSymbolTable_t * symtab = &self->globals->symtab;

    while (p != NULL) {
	if (p->base.type == node_opt || p->base.type == node_iter) {
	    CcSyntax_FindAS(self, p->sub);
	    a = CcSyntax_LeadingAny(self, p->sub);
	    if (a != NULL) {
		CcSyntax_First(self, &s0, p->next);
		CcBitArray_Subtract(((CcNodeANY_t *)a)->set, &s0);
		CcBitArray_Destruct(&s0);
	    }
	} else if (p->base.type == node_alt) {
	    CcBitArray(&s1, symtab->terminals.Count);
	    q = p;
	    while (q != NULL) {
		CcSyntax_FindAS(self, q->sub);
		a = CcSyntax_LeadingAny(self, q->sub);
		if (a != NULL) {
		    CcSyntax_First(self, &s0, q->down);
		    CcBitArray_Or(&s0, &s1);
		    CcBitArray_Subtract(((CcNodeANY_t *)a)->set, &s0);
		    CcBitArray_Destruct(&s0);
		} else {
		    CcSyntax_First(self, &s0, q->sub);
		    CcBitArray_Or(&s1, &s0);
		    CcBitArray_Destruct(&s0);
		}
		q = q->down;
	    }
	    CcBitArray_Destruct(&s1);
	}
	if (p->up) break;
	p = p->next;
    }
}

static void
CcSyntax_CompAnySets(CcSyntax_t * self)
{
    CcArrayListIter_t iter; CcSymbolNT_t * sym;
    CcArrayList_t * nonterminals = &self->globals->symtab.nonterminals;

    for (sym = (CcSymbolNT_t *)CcArrayList_First(nonterminals, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(nonterminals, &iter))
	CcSyntax_FindAS(self, sym->graph);
}

void
CcSyntax_Expected(CcSyntax_t * self, CcBitArray_t * ret,
		  CcNode_t * p, const CcSymbol_t * curSy)
{
    CcSyntax_First(self, ret, p);
    if (CcNode_DelGraph(p))
	CcBitArray_Or(ret, ((const CcSymbolNT_t *)curSy)->follow);
}

void
CcSyntax_Expected0(CcSyntax_t * self, CcBitArray_t * ret,
		   CcNode_t * p, const CcSymbol_t * curSy)
{
    CcSymbolTable_t * symtab = &self->globals->symtab;
    if (p->base.type == node_rslv) CcBitArray(ret, symtab->terminals.Count);
    else CcSyntax_Expected(self, ret, p, curSy);
}

static void
CcSyntax_CompSync(CcSyntax_t * self, CcNode_t * p)
{
    CcNodeSYNC_t * syncp;

    while (p != NULL && !CcBitArray_Get(self->visited, p->base.index)) {
	CcBitArray_Set(self->visited, p->base.index, TRUE);
	if (p->base.type == node_sync) {
	    syncp = (CcNodeSYNC_t *)p;
	    CcsAssert(syncp->set == NULL);
	    CcSyntax_Expected(self, &syncp->setSpace, p->next, self->curSy);
	    syncp->set = &syncp->setSpace;
	    CcBitArray_Set(syncp->set, self->eofSy->kind, TRUE);
	    CcBitArray_Or(self->allSyncSets, syncp->set);
	} else if (p->base.type == node_alt) {
	    CcSyntax_CompSync(self, p->sub);
	    CcSyntax_CompSync(self, p->down);
	} else if (p->base.type == node_opt || p->base.type == node_iter) {
	    CcSyntax_CompSync(self, p->sub);
	}
	p = p->next;
    }
}

static void
CcSyntax_CompSyncSets(CcSyntax_t * self)
{
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    self->allSyncSets = CcBitArray(&self->allSyncSetsSpace,
				   symtab->terminals.Count);
    CcBitArray_Set(self->allSyncSets, self->eofSy->kind, TRUE);
    self->visited = CcBitArray(&self->visitedSpace, self->base.nodes.Count);
    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	self->curSy = (CcSymbol_t *)sym;
	CcSyntax_CompSync(self, sym->graph);
    }
    CcBitArray_Destruct(self->visited); self->visited = NULL;
}

void
CcSyntax_SetupAnys(CcSyntax_t * self)
{
    CcNode_t * p; CcNodeANY_t * pany; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;

    for (p = (CcNode_t *)CcArrayList_First(&self->base.nodes, &iter);
	 p; p = (CcNode_t *)CcArrayList_Next(&self->base.nodes, &iter)) {
	if (p->base.type != node_any) continue;
	pany = (CcNodeANY_t *)p;
	pany->set = &pany->setSpace;
	CcBitArray1(pany->set, symtab->terminals.Count);
	CcBitArray_Set(pany->set, self->eofSy->kind, FALSE);
    }
}

static void
CcSyntax_CompDeletableSymbols(CcSyntax_t * self)
{
    CcsBool_t changed;
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;
    do {
	changed = FALSE;
	for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	     sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	    if (!sym->deletable && sym->graph != NULL &&
		CcNode_DelGraph(sym->graph)) {
		sym->deletable = TRUE; changed = TRUE;
	    }
	}
    } while (changed);
    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	if (sym->deletable) 
	    CcsErrorPool_Warning(self->globals->errpool, NULL,
				 " %s deletable", sym->base.name);
    }
}

static void
CcSyntax_CompSymbolSets(CcSyntax_t * self)
{
    CcSyntax_CompDeletableSymbols(self);
    CcSyntax_CompFirstSets(self);
    CcSyntax_CompFollowSets(self);
    CcSyntax_CompAnySets(self);
    CcSyntax_CompSyncSets(self);
}

static void
GetSingles(CcBitArray_t * singles, CcNode_t * p)
{
    if (p == NULL) return; /* end of graph */
    if (p->base.type == node_nt) {
	if (p->up || CcNode_DelGraph(p->next))
	    CcBitArray_Set(singles, ((CcNodeNT_t *)p)->sym->kind, TRUE);
    } else if (p->base.type == node_alt || p->base.type == node_iter
	       || p->base.type == node_opt) {
	if (p->up || CcNode_DelGraph(p->next)) {
	    GetSingles(singles, p->sub);
	    if (p->base.type == node_alt) GetSingles(singles, p->down);
	}
    }
    if (p->up && CcNode_DelNode(p)) GetSingles(singles, p->next);
}

typedef struct SymbolPair_s SymbolPair_t;
struct SymbolPair_s {
    SymbolPair_t * next;
    int left, right;
};

static CcsBool_t
CcSyntax_NoCircularProductions(CcSyntax_t * self)
{
    CcBitArray_t singles;
    SymbolPair_t * firstPair, * prevPair, * curPair, * curPair0;
    CcSymbolNT_t * sym; CcArrayListIter_t iter; int index;
    CcsBool_t ok, changed, onLeftSide, onRightSide;
    CcSymbol_t * leftsym, * rightsym;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    firstPair = NULL;
    CcBitArray(&singles, ntarr->Count);
    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	CcBitArray_SetAll(&singles, FALSE);
	/* Get nonterminals s such that sym-->s */
	GetSingles(&singles, sym->graph);
	for (index = 0; index < ntarr->Count; ++index) {
	    if (!CcBitArray_Get(&singles, index)) continue;
	    curPair = CcMalloc(sizeof(SymbolPair_t));
	    curPair->left = sym->base.kind;
	    curPair->right = index;
	    curPair->next = firstPair;
	    firstPair = curPair;
	}
    }
    CcBitArray_Destruct(&singles);

    do {
	changed = FALSE;
	prevPair = NULL; curPair = firstPair;
	while (curPair) {
	    onLeftSide = FALSE; onRightSide = FALSE;
	    for (curPair0 = curPair; curPair0; curPair0 = curPair0->next) {
		if (curPair->left == curPair0->right) onRightSide = TRUE;
		if (curPair->right == curPair0->left) onLeftSide = TRUE;
	    }
	    if (onLeftSide && onRightSide) { /* Circular Production found. */
		prevPair = curPair; curPair = curPair->next;
	    } else { /* Remove non-circular nonterminal symbol pair. */
		curPair0 = curPair->next;
		if (prevPair == NULL)  firstPair = curPair0;
		else  prevPair->next = curPair0;
		CcFree(curPair);
		curPair = curPair0;
		changed = TRUE;
	    }
	}
    } while (changed);

    ok = TRUE;
    for (curPair = firstPair; curPair; curPair = curPair0) {
	ok = FALSE;
	leftsym = (CcSymbol_t *)CcArrayList_Get(ntarr, curPair->left);
	rightsym = (CcSymbol_t *)CcArrayList_Get(ntarr, curPair->right);
	CcsErrorPool_Error(self->globals->errpool, NULL, " '%s' --> '%s'",
			   leftsym->name, rightsym->name);
	curPair0 = curPair->next;
	CcFree(curPair);
    }
    return ok;
}

static void
CcSyntax_LL1Error(CcSyntax_t * self, int cond, CcSymbol_t * sym)
{
    const char * s0, * s1, * s2, * s3;
    if (sym) { s0 = "'"; s1 = sym->name; s2 = "' is "; }
    else { s0 = s1 = s2 = ""; }
    switch (cond) {
    case 1: s3 = "start of several alternatives"; break;
    case 2: s3 = "start & successor of deletable structure"; break;
    case 3: s3 = "an ANY node that matches no symbol"; break;
    case 4: s3 = "contents of [...] or {...} must not be deletable"; break;
    default: s3 = ""; break;
    }
    CcsErrorPool_Warning(self->globals->errpool, NULL,
			 "  LL1 warning in '%s': %s%s%s%s",
			 self->curSy->name, s0, s1, s2, s3);
}

static void
CcSyntax_CheckOverlap(CcSyntax_t * self, CcBitArray_t * s1, CcBitArray_t * s2,
		      int cond)
{
    CcSymbolT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * tarr = &symtab->terminals;

    for (sym = (CcSymbolT_t *)CcArrayList_First(tarr, &iter);
	 sym; sym = (CcSymbolT_t *)CcArrayList_Next(tarr, &iter)) {
	if (CcBitArray_Get(s1, sym->base.kind)
	    && CcBitArray_Get(s2, sym->base.kind))
	    CcSyntax_LL1Error(self, cond, (CcSymbol_t *)sym);
    }
}

/* FIX ME */
static void
CcSyntax_CheckAlts(CcSyntax_t * self, CcNode_t * p)
{
    CcBitArray_t s1, s2; CcNode_t * q;
    CcSymbolTable_t * symtab = &self->globals->symtab;

    while (p != NULL) {
	if (p->base.type == node_alt) {
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
	} else if (p->base.type == node_opt || p->base.type == node_iter) {
	    /* E.g. [[...]] */
	    if (CcNode_DelSubGraph(p->sub)) {
		CcSyntax_LL1Error(self, 4, NULL);
	    } else {
		CcSyntax_Expected0(self, &s1, p->sub, self->curSy);
		CcSyntax_Expected(self, &s2, p->next, self->curSy);
		CcSyntax_CheckOverlap(self, &s1, &s2, 2);
		CcSyntax_CheckAlts(self, p->sub);
		CcBitArray_Destruct(&s1);
		CcBitArray_Destruct(&s2);
	    }
	} else if (p->base.type == node_any) {
	    /* E.g. {ANY} ANY or [ANY] ANY */
	    if (CcBitArray_Elements(((CcNodeANY_t *)p)->set) == 0)
		CcSyntax_LL1Error(self, 3, NULL);
	}
	if (p->up) break;
	p = p->next;
    }
}

static void
CcSyntax_CheckLL1(CcSyntax_t * self)
{
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
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
    CcSymbolTable_t * symtab = &self->globals->symtab;

    while (p != NULL) {
	if (p->base.type == node_alt) {
	    CcBitArray(&expected, symtab->terminals.Count);
	    for (q = p; q != NULL; q = q->down) {
		CcSyntax_Expected0(self, &fs, q->sub, self->curSy);
		CcBitArray_Or(&expected, &fs);
		CcBitArray_Destruct(&fs);
	    }
	    CcBitArray(&soFar, symtab->terminals.Count);
	    for (q = p; q != NULL; q = q->down) {
		if (q->sub->base.type == node_rslv) {
		    CcSyntax_Expected(self, &fs, q->sub->next, self->curSy);
		    if (CcBitArray_Intersect(&fs, &soFar))
			CcsErrorPool_Warning(self->globals->errpool, NULL,
					     "Resolver will never be evaluated. "
					     "Place it at previous conflicting alternative.");
		    if (!CcBitArray_Intersect(&fs, &expected))
			CcsErrorPool_Warning(self->globals->errpool, NULL,
					     "Misplaced resolver: no LL(1) conflict.");
		    CcBitArray_Destruct(&fs);
		} else {
		    CcSyntax_Expected(self, &fs, q->sub, self->curSy);
		    CcBitArray_Or(&soFar, &fs);
		    CcBitArray_Destruct(&fs);
		}
		CcSyntax_CheckRes(self, q->sub, TRUE);
	    }
	    CcBitArray_Destruct(&expected); CcBitArray_Destruct(&soFar);
	} else if (p->base.type == node_iter || p->base.type == node_opt) {
	    if (p->sub->base.type == node_rslv) {
		CcSyntax_First(self, &fs, p->sub->next);
		CcSyntax_Expected(self, &fsNext, p->next, self->curSy);
		if (!CcBitArray_Intersect(&fs, &fsNext))
		    CcsErrorPool_Warning(self->globals->errpool, NULL,
					 "Misplaced resolver: no LL(1) conflict.");
	    }
	    CcSyntax_CheckRes(self, p->sub, TRUE);
	} else if (p->base.type == node_rslv) {
	    if (!rslvAllowed)
		CcsErrorPool_Warning(self->globals->errpool, NULL,
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
    CcSymbolTable_t * symtab = &self->globals->symtab;
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
    CcSymbolTable_t * symtab = &self->globals->symtab;
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbolNT_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (sym->graph == NULL) {
	    complete = FALSE;
	    CcsErrorPool_Error(self->globals->errpool, NULL,
			       "No production for %s", sym->base.name);
	}
    }
    return complete;
}

static void
CcSyntax_MarkReachedNts(CcSyntax_t * self, CcNode_t * p)
{
    while (p != NULL) {
	if (p->base.type == node_nt) {
	    CcNodeNT_t * p0 = (CcNodeNT_t *)p;
	    CcSymbolNT_t * sym = (CcSymbolNT_t *)p0->sym;
	    if (!CcBitArray_Get(self->visited, sym->base.kind)) {
		/* new nt reached */
		CcBitArray_Set(self->visited, sym->base.kind, TRUE);
		CcSyntax_MarkReachedNts(self, sym->graph);
	    }
	} else if (p->base.type == node_alt || p->base.type == node_iter
		   || p->base.type == node_opt) {
	    CcSyntax_MarkReachedNts(self, p->sub);
	    if (p->base.type == node_alt)
		CcSyntax_MarkReachedNts(self, p->down);
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
    CcSymbolTable_t * symtab = &self->globals->symtab;

    self->visited = CcBitArray(&self->visitedSpace,
			       symtab->nonterminals.Count);
    CcBitArray_Set(self->visited, self->gramSy->kind, TRUE);
    CcSyntax_MarkReachedNts(self, ((CcSymbolNT_t *)self->gramSy)->graph);
    for (idx = 0; idx < symtab->nonterminals.Count; ++idx) {
	sym = (CcSymbol_t *)CcArrayList_Get(&symtab->nonterminals, idx);
	if (!CcBitArray_Get(self->visited, sym->kind)) {
	    ok = FALSE;
	    CcsErrorPool_Error(self->globals->errpool, NULL,
			       " '%s' cannot be reached", sym->name);
	}
    }
    return ok;
}

/*--------- check if every nts can be derived to terminals  ------------ */
/* true if graph can be derived to terminals */
CcsBool_t
CcSyntax_IsTerm(CcSyntax_t *self, CcNode_t * p, CcBitArray_t * mark) {
    while (p != NULL) {
	if (p->base.type == node_nt &&
	    !CcBitArray_Get(mark, (((CcNodeNT_t *)p)->sym)->base.index))
	    return FALSE;
	if (p->base.type == node_alt && !CcSyntax_IsTerm(self, p->sub, mark) &&
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
    CcSymbolNT_t * sym; CcArrayListIter_t iter;
    CcsBool_t changed, ok = TRUE;
    CcSymbolTable_t * symtab = &self->globals->symtab;
    CcArrayList_t * ntarr = &symtab->nonterminals;

    CcBitArray(&mark, ntarr->Count);
    do {
	changed = FALSE;
	for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	     sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	    if (!CcBitArray_Get(&mark, sym->base.kind) &&
		CcSyntax_IsTerm(self, sym->graph, &mark)) {
		CcBitArray_Set(&mark, sym->base.kind, TRUE);
		changed = TRUE;
	    }
	}
    } while (changed);
    for (sym = (CcSymbolNT_t *)CcArrayList_First(ntarr, &iter);
	 sym; sym = (CcSymbolNT_t *)CcArrayList_Next(ntarr, &iter)) {
	if (!CcBitArray_Get(&mark, sym->base.kind)) {
	    ok = FALSE;
	    CcsErrorPool_Error(self->globals->errpool, NULL,
			       " %s cannot be derived to terminals",
			       sym->base.name);
	}
    }
    CcBitArray_Destruct(&mark);
    return ok;
}

static CcsBool_t
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

static void CcSyntax_Errors(CcSyntax_t * self);
CcsBool_t
CcSyntax_Finish(CcSyntax_t * self)
{
    CcSyntax_CompSymbolSets(self);
    if (!CcSyntax_GrammarOk(self)) return FALSE;
    CcSyntax_Errors(self);
    return TRUE;
}

static const CcObjectType_t CcSyntaxErrorType = {
    sizeof(CcSyntaxError_t), "CcSyntaxError_t", CcObject_Destruct
};

static void
CcSyntax_Errors(CcSyntax_t * self)
{
    CcArrayListIter_t iter;
    const CcSymbol_t * sym;
    CcSyntaxError_t * error;
    const CcArrayList_t * terminals = &self->globals->symtab.terminals;
    for (sym = (const CcSymbol_t *)CcArrayList_FirstC(terminals, &iter);
	 sym; sym = (const CcSymbol_t *)CcArrayList_NextC(terminals, &iter)) {
	error = (CcSyntaxError_t *)
	    CcArrayList_New(&self->errors, CcObject(&CcSyntaxErrorType));
	error->type = cet_t;
	error->sym = sym;
    }
}

int
CcSyntax_AltError(CcSyntax_t * self, const CcSymbol_t * sym)
{
    CcSyntaxError_t * error = (CcSyntaxError_t *)
	CcArrayList_New(&self->errors, CcObject(&CcSyntaxErrorType));
    error->type = cet_alt;
    error->sym = sym;
    return error->base.index;
}

int
CcSyntax_SyncError(CcSyntax_t * self, const CcSymbol_t * sym)
{
    CcSyntaxError_t * error = (CcSyntaxError_t *)
	CcArrayList_New(&self->errors, CcObject(&CcSyntaxErrorType));
    error->type = cet_sync;
    error->sym = sym;
    return error->base.index;
}

#define SZ_SYMSET  64

void
CcSyntaxSymSet(CcSyntaxSymSet_t * self)
{
    self->start = self->used = CcMalloc(sizeof(CcBitArray_t) * SZ_SYMSET);
    self->last = self->start + SZ_SYMSET;
}

int
CcSyntaxSymSet_New(CcSyntaxSymSet_t * self, const CcBitArray_t * s)
{
    CcBitArray_t * cur;
    for (cur = self->start; cur < self->used; ++cur)
	if (CcBitArray_Equal(cur, s)) return cur - self->start;
    if (self->used >= self->last) {
	cur = CcRealloc(self->start,
			sizeof(CcBitArray_t) * (self->last - self->start) * 2);
	self->used = cur + (self->used - self->start);
	self->last = cur + (self->last - self->start) * 2;
	self->start = cur;
    }
    cur = self->used++;
    CcBitArray_Clone(cur, s);
    return cur - self->start;
}

void
CcSyntaxSymSet_Destruct(CcSyntaxSymSet_t * self)
{
    CcBitArray_t * cur;
    for (cur = self->start; cur < self->used; ++cur)
	CcBitArray_Destruct(cur);
    CcFree(self->start);
}
