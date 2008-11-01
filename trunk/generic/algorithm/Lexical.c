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
#include  "Lexical.h"
#include  "ArrayList.h"
#include  "BitArray.h"
#include  "Globals.h"
#include  "lexical/Action.h"
#include  "lexical/CharSet.h"
#include  "lexical/CharClass.h"
#include  "lexical/Comment.h"
#include  "lexical/Melted.h"
#include  "lexical/Nodes.h"
#include  "lexical/State.h"
#include  "lexical/Target.h"

/* SZ_LITERALS is a prime number, auto-extending is not supported now */
#define  SZ_LITERALS 127

static CcAction_t *
CcLexical_FindAction(CcLexical_t * self, CcState_t *state, int ch);
static void
CcLexical_GetTargetStates(CcLexical_t * self, CcAction_t * a,
			  CcBitArray_t ** targets, CcSymbolT_t ** endOf,
			  CcsBool_t * ctx);
static CcMelted_t *
CcLexical_NewMelted(CcLexical_t * self, CcBitArray_t * set, CcState_t * state);
static CcBitArray_t * CcLexical_MeltedSet(CcLexical_t * self, int nr);
static CcMelted_t *
CcLexical_StateWithSet(CcLexical_t * self, CcBitArray_t * s);

CcLexical_t *
CcLexical(CcLexical_t * self, CcGlobals_t * globals)
{
    self->globals = globals;
    self->ignored = CcCharSet();
    CcArrayList(&self->nodes);
    CcArrayList(&self->classes);
    CcHashTable(&self->literals, SZ_LITERALS);
    return self;
}

void
CcLexical_Destruct(CcLexical_t * self)
{
    CcHashTable_Destruct(&self->literals);
    CcArrayList_Destruct(&self->classes);
    CcArrayList_Destruct(&self->nodes);
    CcCharSet_Destruct(self->ignored);
}

void
CcLexical_MakeFirstAlt(CcLexical_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeFirstAlt(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcLexical_MakeAlternative(CcLexical_t * self, CcGraph_t * g1, CcGraph_t * g2)
{
    CcNode_t * p = CcGraph_MakeAlternative(g1, g2, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcLexical_MakeIteration(CcLexical_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeIteration(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcLexical_MakeOption(CcLexical_t * self, CcGraph_t * g)
{
    CcNode_t * p = CcGraph_MakeOption(g, self->nodes.Count);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
}

void
CcLexical_DeleteNodes(CcLexical_t * self)
{
    CcArrayList_Clear(&self->nodes);
    self->dummyNode = CcLexical_NewNodeEps(self);
}

CcGraph_t *
CcLexical_StrToGraph(CcLexical_t * self, const char * str, CcsToken_t * t)
{
    CcGraph_t * g; CcNode_t * p;
    const char * cur, * slast;
    char * s = CcsUnescape(str);
    if (strlen(s) == 0)
	CcsGlobals_SemErr(&self->globals->base, t, "empty token not allowed");
    g = CcGraph();
    g->r = self->dummyNode;
    slast = s + strlen(s);
    for (cur = s; cur < slast; ++cur) {
	p = CcLexical_NewNodeChr(self, CcsUTF8GetCh(&cur, slast));
	g->r->next = p; g->r = p;
    }
    g->l = self->dummyNode->next; self->dummyNode->next = NULL;
    CcFree(s);
    return g;
}

void
CcLexical_SetContextTrans(CcLexical_t * self, CcNode_t * p)
{
    const CcNodeType_t * ptype;
    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_chr) {
	    ((CcNodeChr_t *)p)->code = node_contextTrans;
	} else if (ptype == node_clas) {
	    ((CcNodeClas_t *)p)->code = node_contextTrans;
	} else if (ptype == node_opt || ptype == node_iter) {
	    CcLexical_SetContextTrans(self, p->sub);
	} else if (ptype == node_alt) {
	    CcLexical_SetContextTrans(self, p->sub);
	    CcLexical_SetContextTrans(self, p->down);
	}
	if (p->up) break;
	p = p->next;
    }
}

CcNode_t *
CcLexical_NewNodeEps(CcLexical_t * self)
{
    CcNode_t * p  = CcNode(node_eps, 0);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
    return p;
}

CcNode_t *
CcLexical_NewNodeChr(CcLexical_t * self, int ch)
{
    CcNodeChr_t * p;
    p = CcNodeChr(self->nodes.Count, ch);
    CcArrayList_Add(&self->nodes, (CcObject_t *)p);
    return (CcNode_t *)p;
}

CcCharClass_t *
CcLexical_NewCharClass(CcLexical_t * self, const char * name, CcCharSet_t * s)
{
    CcCharClass_t * c = CcCharClass(self->classes.Count, name, s);
    CcArrayList_Add(&self->classes, (CcObject_t *)c);
    return c;
}

CcCharClass_t *
CcLexical_FindCharClassN(CcLexical_t * self, const char * name)
{
    int idx; CcCharClass_t * c;
    for (idx = 0; idx < self->classes.Count; ++idx) {
	c = (CcCharClass_t *)CcArrayList_Get(&self->classes, idx);
	if (!strcmp(name, c->name)) return c;
    }
    return NULL;
}

CcCharClass_t *
CcLexical_FindCharClassC(CcLexical_t * self, const CcCharSet_t * s)
{
    int idx; CcCharClass_t * c;
    for (idx = 0; idx < self->classes.Count; ++idx) {
	c = (CcCharClass_t *)CcArrayList_Get(&self->classes, idx);
	if (CcCharSet_Equals(s, c->set)) return c;
    }
    return NULL;
}

CcCharSet_t *
CcLexical_CharClassSet(CcLexical_t * self, int idx)
{
    return ((CcCharClass_t *)CcArrayList_Get(&self->classes, idx))->set;
}

CcCharSet_t *
CcLexical_ActionSymbols(CcLexical_t * self, CcAction_t * action)
{
    CcCharSet_t * s;

    if (action->typ == node_clas) {
	s = CcCharSet_Clone(CcLexical_CharClassSet(self, action->sym));
    } else {
	s = CcCharSet();
	CcCharSet_Set(s, action->sym);
    }
    return s;
}

void
CcLexical_ActionShiftWith(CcLexical_t * self,
			  CcAction_t * action, CcCharSet_t * s)
{
    CcCharClass_t * c;

    if (CcCharSet_Elements(s) == 1) {
	action->typ = node_chr; action->sym = CcCharSet_First(s);
    } else {
	CcLexical_FindCharClassC(self, s);
	if (c == NULL) c = CcLexical_NewCharClass(self, "#", s);
	action->typ = node_clas; action->sym = c->n;
    }
}

static CcState_t *
CcLexical_NewState(CcLexical_t * self)
{
    CcState_t * s = CcState(++self->lastStateNr);
    if (self->firstState == NULL) self->firstState = s;
    else self->lastState->next = s;
    self->lastState = s;
    return s;
}

static void
CcLexical_NewTransition(CcLexical_t * self, CcState_t * from, CcState_t * to,
			const CcNodeType_t * typ, int sym, int tc)
{
    CcTarget_t * t; CcAction_t * a;

    if (to == self->firstState)
	CcsGlobals_SemErr(&self->globals->base, NULL,
			  "token must not start with an iteration");
    t = CcTarget(to);
    a = CcAction(typ, sym, tc); a->target = t;
    State_AddAction(from, a);
    if (typ == node_clas) self->curSy->tokenKind = symbol_classToken;
}

static void
CcLexical_CombineShifts(CcLexical_t * self)
{
    CcState_t * state;
    CcAction_t * a, * b, * c;
    CcCharSet_t * seta, * setb;

    for (state = self->firstState; state != NULL; state = state->next) {
	for (a = state->firstAction; a != NULL; a = a->next) {
	    b = a->next;
	    while (b != NULL)
		if (a->target->state == b->target->state && a->tc == b->tc) {
		    seta = CcLexical_ActionSymbols(self, a);
		    setb = CcLexical_ActionSymbols(self, b);
		    CcCharSet_Or(seta, setb);
		    CcLexical_ActionShiftWith(self, a, seta);
		    c = b; b = b->next; State_DetachAction(state, c);
		} else {
		    b = b->next;
		}
	}
    }
}

static void
CcLexical_FindUsedStates(CcLexical_t * self, CcState_t * state,
			 CcBitArray_t * used)
{
    CcAction_t * a;

    if (CcBitArray_Get(used, state->nr)) return;
    CcBitArray_Set(used, state->nr, TRUE);
    for (a = state->firstAction; a != NULL; a = a->next)
	CcLexical_FindUsedStates(self, a->target->state, used);
}

static void
CcLexical_DeleteRedundantStates(CcLexical_t * self)
{
    CcState_t * s1, * s2, * state;
    CcBitArray_t used;
    CcAction_t * a;
    CcState_t ** newState = (CcState_t **)
	CcMalloc(sizeof(CcState_t *) * (self->lastStateNr + 1));

    CcBitArray(&used, self->lastStateNr + 1);

    CcLexical_FindUsedStates(self, self->firstState, &used);
    for (s1 = self->firstState->next; s1 != NULL; s1 = s1->next)
	if (CcBitArray_Get(&used, s1->nr) && s1->endOf != NULL &&
	    s1->firstAction == NULL && !(s1->ctx))
	    for (s2 = s1->next; s2 != NULL; s2 = s2->next)
		if (CcBitArray_Get(&used, s2->nr) && s1->endOf == s2->endOf &&
		    s2->firstAction == NULL && !(s2->ctx)) {
		    CcBitArray_Set(&used, s2->nr, FALSE);
		    newState[s2->nr] = s1;
		}

    for (state = self->firstState; state != NULL; state = state->next)
	if (CcBitArray_Get(&used, state->nr))
	    for (a = state->firstAction; a != NULL; a = a->next)
		if (!CcBitArray_Get(&used, a->target->state->nr))
		    a->target->state = newState[a->target->state->nr];
    /* delete unused states */
    self->lastState = self->firstState; self->lastStateNr = 0;
    for (state = self->firstState->next; state != NULL; state = state->next)
	if (CcBitArray_Get(&used, state->nr)) {
	    state->nr = ++self->lastStateNr; self->lastState = state;
	} else {
	    self->lastState->next = state->next;
	}
    CcFree(newState);
    CcBitArray_Destruct(&used);
}

static CcState_t *
CcLexical_TheState(CcLexical_t * self, CcNode_t * p)
{
    CcState_t * state;

    if (p == NULL) {
	state = CcLexical_NewState(self);
	state->endOf = self->curSy;
	return state;
    }
    return p->state;
}

static void
CcLexical_Step(CcLexical_t * self, CcState_t * from, CcNode_t * p, CcBitArray_t * stepped)
{
    CcBitArray_t newStepped;
    const CcNodeType_t * ptype;

    if (p == NULL) return;
    CcBitArray_Set(stepped, p->n, TRUE);
    ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_clas) {
	CcLexical_NewTransition(self, from, CcLexical_TheState(self, p->next),
				ptype, ((CcNodeClas_t *)p)->clas,
				((CcNodeClas_t *)p)->code);
    } else if (ptype == node_chr) {
	CcLexical_NewTransition(self, from, CcLexical_TheState(self, p->next),
				ptype, ((CcNodeChr_t *)p)->chr,
				((CcNodeChr_t *)p)->code);
    } else if (ptype == node_alt) {
	CcLexical_Step(self, from, p->sub, stepped);
	CcLexical_Step(self, from, p->down, stepped);
    } else if (ptype == node_iter || ptype == node_opt) {
	if (p->next != NULL && CcBitArray_Get(stepped, p->next->n))
	    CcLexical_Step(self, from, p->next, stepped);
	CcLexical_Step(self, from, p->sub, stepped);
	if ((ptype == node_iter) && (p->state != from)) {
	    CcBitArray(&newStepped, self->nodes.Count);
	    CcLexical_Step(self, p->state, p, &newStepped);
	    CcBitArray_Destruct(&newStepped);
	}
    }
}

static void
CcLexical_NumberNodes(CcLexical_t * self, CcNode_t * p,
		      CcState_t * state, CcsBool_t renumIter)
{
    const CcNodeType_t * ptype = (const CcNodeType_t *)p->base.type;

    if (p == NULL) return;
    if (p->state != NULL) return;
    if ((state == NULL) || (ptype == node_iter && renumIter))
	state = CcLexical_NewState(self);
    p->state = state;
    if (CcSyntax_DelGraph(&self->globals->syntax, p))
	state->endOf = self->curSy;

    if (ptype == node_clas || ptype == node_chr) {
	CcLexical_NumberNodes(self, p->next, NULL, FALSE);
    } else if (ptype == node_opt) {
	CcLexical_NumberNodes(self, p->next, NULL, FALSE);
	CcLexical_NumberNodes(self, p->sub, state, TRUE);
    } else if (ptype == node_iter) {
	CcLexical_NumberNodes(self, p->next, state, TRUE);
	CcLexical_NumberNodes(self, p->sub, state, TRUE);
    } else if (ptype == node_alt) {
	CcLexical_NumberNodes(self, p->next, NULL, FALSE);
	CcLexical_NumberNodes(self, p->sub, state, TRUE);
	CcLexical_NumberNodes(self, p->down, state, renumIter);
    }
}

static void
CcLexical_FindTrans(CcLexical_t * self, CcNode_t * p, CcsBool_t start, CcBitArray_t * marked)
{
    CcBitArray_t stepped;
    const CcNodeType_t * ptype;

    if (p == NULL || CcBitArray_Get(marked, p->n)) return;
    CcBitArray_Set(marked, p->n, TRUE);
    if (start) {
	CcBitArray(&stepped, self->nodes.Count);
	/* start of group of equally numbered nodes */
	CcLexical_Step(self, p->state, p, &stepped);
	CcBitArray_Destruct(&stepped);
    }

    ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_clas || ptype == node_chr) {
	CcLexical_FindTrans(self, p->next, TRUE, marked);
    } else if (ptype == node_opt) {
	CcLexical_FindTrans(self, p->next, TRUE, marked);
	CcLexical_FindTrans(self, p->sub, FALSE, marked);
    } else if (ptype == node_iter) {
	CcLexical_FindTrans(self, p->next, FALSE, marked);
	CcLexical_FindTrans(self, p->sub, FALSE, marked);
    } else if (ptype == node_alt) {
	CcLexical_FindTrans(self, p->sub, FALSE, marked);
	CcLexical_FindTrans(self, p->down, FALSE, marked);
    }
}

void
CcLexical_ConvertToStates(CcLexical_t * self, CcNode_t * p, CcSymbolT_t * sym)
{
    CcBitArray_t stepped, marked;
    const CcNodeType_t * ptype;

    self->curGraph = p; self->curSy = sym;
    if (CcSyntax_DelGraph(&self->globals->syntax, self->curGraph))
	CcsGlobals_SemErr(&self->globals->base, NULL, "token might be empty");
    CcLexical_NumberNodes(self, self->curGraph, self->firstState, TRUE);

    CcBitArray(&marked, self->nodes.Count);
    CcLexical_FindTrans(self, self->curGraph, TRUE, &marked);
    CcBitArray_Destruct(&marked);

    ptype = (const CcNodeType_t *)p->base.type;
    if (ptype == node_iter) {
	CcBitArray(&stepped, self->nodes.Count);
	CcLexical_Step(self, self->firstState, p, &stepped);
	CcBitArray_Destruct(&stepped);
    }
}

/* match string against current automaton; store it either as a
 * fixedToken or as a litToken */
void
CcLexical_MatchLiteral(CcLexical_t * self, const char * s, CcSymbolT_t * sym)
{
    char * s0 = CcsUnescape(s);
    const char * scur, * slast;
    CcState_t * to, * state = self->firstState;
    CcAction_t * a = NULL;
    CcSymbolT_t * matchedSym;

    /* Try to match s against existing CcLexical. */
    scur = s0; slast = scur + strlen(s0);
    while (scur < slast) {
	a = CcLexical_FindAction(self, state, CcsUTF8GetCh(&scur, slast));
	if (a == NULL) break;
	state = a->target->state;
    }

    /* if s was not totally consumed or leads to a non-final state => make
     * new CcLexical from it */
    if (*scur || state->endOf == NULL) {
	state = self->firstState; scur = s0; a = NULL;
	self->dirtyLexical = TRUE;
    }
    while (scur < slast) { /* make new CcLexical for s0[i..len-1] */
	to = CcLexical_NewState(self);
	CcLexical_NewTransition(self, state, to, node_chr,
				CcsUTF8Get(&scur, slast),
				node_normalTrans);
	state = to;
    }
    CcFree(s0);

    matchedSym = state->endOf;
    if (state->endOf == NULL) {
	state->endOf = sym;
    } else if (matchedSym->tokenKind == symbol_fixedToken ||
	       (a != NULL && a->tc == node_contextTrans)) {
	/* s matched a token with a fixed definition or a token with
	 * an appendix that will be cut off */
	CcsGlobals_SemErr(&self->globals->base, NULL,
		      "tokens %ls and %ls cannot be distinguished",
		      sym->base.name, matchedSym->base.name);
    } else { /* matchedSym == classToken || classLitToken */
	matchedSym->tokenKind = symbol_classLitToken;
	sym->tokenKind = symbol_litToken;
    }
}

static void
CcLexical_SplitActions(CcLexical_t * self, CcState_t * state,
		       CcAction_t * a, CcAction_t * b)
{
    CcAction_t * c; CcCharSet_t * seta, * setb, * setc;

    seta = CcLexical_ActionSymbols(self, a);
    setb = CcLexical_ActionSymbols(self, b);
    if (CcCharSet_Equals(seta, setb)) {
	Action_AddTargets(a, b);
	State_DetachAction(state, b);
    } else if (CcCharSet_Includes(seta, setb)) {
	setc = CcCharSet_Clone(seta); CcCharSet_Subtract(setc, setb);
	Action_AddTargets(b, a);
	CcLexical_ActionShiftWith(self, a, setc);
	CcCharSet_Destruct(setc);
    } else if (CcCharSet_Includes(setb, seta)) {
	setc = CcCharSet_Clone(setb); CcCharSet_Subtract(setc, seta);
	Action_AddTargets(a, b);
	CcLexical_ActionShiftWith(self, b, setc);
	CcCharSet_Destruct(setc);
    } else {
	setc = CcCharSet_Clone(seta); CcCharSet_And(setc, setb);
	CcCharSet_Subtract(seta, setc);
	CcCharSet_Subtract(setb, setc);
	CcLexical_ActionShiftWith(self, a, seta);
	CcLexical_ActionShiftWith(self, b, setb);
	/* typ and sym are set in ShiftWith */
	c = CcAction(0, 0, node_normalTrans);
	CcAction_AddTargets(c, a);
	CcAction_AddTargets(c, b);
	CcLexical_ActionShiftWith(self, c, setc);
	CcState_AddAction(state, c);
	CcCharSet_Destruct(setc);
    }
}

static CcsBool_t
CcLexical_Overlap(CcLexical_t * self, CcAction_t * a, CcAction_t * b) {
    CcCharSet_t * seta, * setb;
    if (a->typ == node_chr) {
	if (b->typ == node_chr) return (a->sym == b->sym);
	setb = CcLexical_CharClassSet(self, b->sym);
	return CcCharSet_Get(setb, a->sym);
    } else {
	seta = CcLexical_CharClassSet(self, a->sym);
	if (b->typ == node_chr) return CcCharSet_Get(seta, b->sym);
	setb = CcLexical_CharClassSet(self, b->sym);
	return CcCharSet_Intersects(seta, setb);
    }
}

/* return true if actions were split */
static CcsBool_t
CcLexical_MakeUnique(CcLexical_t * self, CcState_t *state) {
    CcAction_t * a, * b;
    CcsBool_t changed = FALSE;

    for (a = state->firstAction; a != NULL; a = a->next)
	for (b = a->next; b != NULL; b = b->next)
	    if (CcLexical_Overlap(self, a, b)) {
		CcLexical_SplitActions(self, state, a, b);
		changed = TRUE;
	    }
    return changed;
}

static void
CcLexical_MeltStates(CcLexical_t * self, CcState_t * state) {
    CcsBool_t changed, ctx;
    CcBitArray_t * targets;
    CcSymbolT_t * endOf;
    CcAction_t * action;
    CcMelted_t * melt;
    CcState_t * s;
    CcTarget_t * targ;

    for (action = state->firstAction; action != NULL; action = action->next) {
	if (action->target->next != NULL) {
	    CcLexical_GetTargetStates(self, action, &targets, &endOf, &ctx);
	    melt = CcLexical_StateWithSet(self, targets);
	    if (melt == NULL) {
		s = CcLexical_NewState(self); s->endOf = endOf; s->ctx = ctx;
		for (targ = action->target; targ != NULL; targ = targ->next)
		    State_MeltWith(s, targ->state);
		do { changed = CcLexical_MakeUnique(self, s); } while (changed);
		melt = CcLexical_NewMelted(self, targets, s);
	    }
	    action->target->next = NULL;
	    action->target->state = melt->state;
	}
    }
}

static void
CcLexical_FindCtxStates(CcLexical_t * self) {
    CcState_t * state;
    CcAction_t * a;

    for (state = self->firstState; state != NULL; state = state->next)
	for (a = state->firstAction; a != NULL; a = a->next)
	    if (a->tc == node_contextTrans) a->target->state->ctx = TRUE;
}

void
CcLexical_MakeDeterministic(CcLexical_t * self) {
    CcState_t * state;
    CcsBool_t  changed;

    self->lastSimState = self->lastState->nr;
    /* heuristic for set size in CcMelted.set */
    self->maxStates = 2 * self->lastSimState;
    CcLexical_FindCtxStates(self);
    for (state = self->firstState; state != NULL; state = state->next)
	do { changed = CcLexical_MakeUnique(self, state); } while (changed);
    for (state = self->firstState; state != NULL; state = state->next)
	CcLexical_MeltStates(self, state);
    CcLexical_DeleteRedundantStates(self);
    CcLexical_CombineShifts(self);
}

/* ------------------------- melted states ------------------------------ */
static CcMelted_t *
CcLexical_NewMelted(CcLexical_t * self, CcBitArray_t * set, CcState_t * state)
{
    CcMelted_t * m = CcMelted(set, state);
    m->next = self->firstMelted; self->firstMelted = m;
    return m;
}

static CcBitArray_t *
CcLexical_MeltedSet(CcLexical_t * self, int nr)
{
    CcMelted_t * m = self->firstMelted;
    while (m != NULL) {
	if (m->state->nr == nr) return m->set;
	else m = m->next;
    }
    /*Errors::Exception("-- compiler error in CcMelted::Set");*/
    /*throw new Exception("-- compiler error in CcMelted::Set");*/
    return NULL;
}

CcMelted_t *
CcLexical_StateWithSet(CcLexical_t * self, CcBitArray_t * s)
{
    CcMelted_t * m;
    for (m = self->firstMelted; m != NULL; m = m->next)
	if (CcBitArray_Equal(s, m->set)) return m;
    return NULL;
}

/* ---------------------------- actions -------------------------------- */
static CcAction_t *
CcLexical_FindAction(CcLexical_t * self, CcState_t *state, int ch) {
    CcAction_t * a; CcCharSet_t * s;
    const CcNodeType_t * atype;

    for (a = state->firstAction; a != NULL; a = a->next) {
	if (a->typ == node_chr && ch == a->sym) return a;
	else if (a->typ == node_clas) {
	    s  = CcLexical_CharClassSet(self, a->sym);
	    if (CcCharSet_Get(s, ch)) return a;
	}
    }
    return NULL;
}

void
CcLexical_GetTargetStates(CcLexical_t * self, CcAction_t * a,
			  CcBitArray_t ** targets, CcSymbolT_t ** endOf,
			  CcsBool_t * ctx)
{
    CcTarget_t * t; int stateNr;
    /* compute the set of target states */
    *targets = CcBitArray(CcMalloc(sizeof(CcBitArray_t)), self->maxStates);
    *endOf = NULL; *ctx = FALSE;
    for (t = a->target; t != NULL; t = t->next) {
	stateNr = t->state->nr;
	if (stateNr <= self->lastSimState) {
	    CcBitArray_Set(*targets, stateNr, TRUE);
	} else {
	    CcBitArray_Or(*targets, CcLexical_MeltedSet(self, stateNr));
	}
	if (t->state->endOf != NULL) {
	    if (*endOf == NULL || *endOf == t->state->endOf)
		*endOf = t->state->endOf;
	    else {
		CcsGlobals_SemErr(&self->globals->base, NULL, 
				  "Tokens %s and %s cannot be distinguished\n",
				  (*endOf)->base.name,
				  t->state->endOf->base.name);
	    }
	}
	if (t->state->ctx) {
	    *ctx = TRUE;
	    /* The following check seems to be unnecessary. It reported an error
	     * if a symbol + context was the prefix of another symbol, e.g.
	     *   s1 = "a" "b" "c".
	     *   s2 = "a" CONTEXT("b").
	     * But this is ok.
	     * if (t.state.endOf != null) {
	     *   Console.WriteLine("Ambiguous context clause");
	     *	 Errors.count++;
	     * } */
	}
    }
}

/* ------------------------ comments -------------------------------- */
static void
CcLexical_CommentStr(CcLexical_t * self, const CcsToken_t * token,
		     int * output, CcNode_t * p) {
    CcCharSet_t * set;
    const CcNodeType_t * ptype;
    int * cur = output;

    *cur = 0;
    while (p != NULL) {
	ptype = (const CcNodeType_t *)p->base.type;
	if (ptype == node_chr) {
	    *cur++ = ((CcNodeChr_t *)p)->chr;
	} else if (ptype == node_clas) {
	    set = CcLexical_CharClassSet(self, ((CcNodeClas_t *)p)->clas);
	    if (CcCharSet_Elements(set) != 1)
		CcsGlobals_SemErr(&self->globals->base, token,
				  "character set contains more than 1 character");
	    *cur++ = CcCharSet_First(set);
	} else {
	    CcsGlobals_SemErr(&self->globals->base, token,
			      "comment delimiters may not be structured");
	}
	if (cur - output > 2) {
	    CcsGlobals_SemErr(&self->globals->base, token,
			      "comment delimiters must be 1 or 2 characters long");
	    cur = output; *cur++ = '?';
	    break;
	}
	p = p->next;
    }
    *cur = 0;
}

void
CcLexical_NewComment(CcLexical_t * self, const CcsToken_t * token,
		     CcNode_t * from, CcNode_t * to, CcsBool_t nested)
{
    int start[3], stop[3];
    CcComment_t * c;

    CcLexical_CommentStr(self, token, start, from);
    CcLexical_CommentStr(self, token, stop, to);
    c = CcComment(start, stop, nested);
    c->next = self->firstComment; self->firstComment = c;
}
