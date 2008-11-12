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
#include  "lexical/Action.h"
#include  "lexical/CharClass.h"
#include  "lexical/CharSet.h"
#include  "lexical/Target.h"
#include  "lexical/State.h"

CcAction_t *
CcAction(const CcCharSet_t * s, CcNode_Transition_t tc,
	 CcArrayList_t * classes)
{
    CcAction_t * self = CcMalloc(sizeof(CcAction_t));
    self->classes = classes;
    self->tc = tc;
    self->target = NULL;
    self->next = NULL;
    CcAction_SetShift(self, s);
    return self;
}

CcAction_t *
CcAction_Clone(const CcAction_t * action)
{
    CcAction_t * self = CcMalloc(sizeof(CcAction_t));
    self->classes = action->classes;
    self->tc = action->tc;
    self->target = NULL;
    self->next = NULL;
    self->single = action->single;
    if (self->single) self->u.chr = action->u.chr;
    else self->u.set = action->u.set;
    CcAction_AddTargets(self, action);
    return self;
}

void
CcAction_Destruct(CcAction_t * self)
{
    CcTarget_t * cur, * next;

    for (cur = self->target; cur; cur = next) {
	next = cur->next;
	CcTarget_Destruct(cur);
    }
    CcFree(self);
}

int
CcAction_ShiftSize(CcAction_t * self)
{
    if (self->single) return 1;
    return CcCharSet_Elements(self->u.set);
}

CcCharSet_t *
CcAction_GetShift(CcAction_t * self)
{
    CcCharSet_t * s;
    if (self->single) {
	s = CcCharSet();
	CcCharSet_Set(s, self->u.chr);
    } else {
	s = CcCharSet_Clone(self->u.set);
    }
    return s;
}

void
CcAction_SetShift(CcAction_t * self, const CcCharSet_t * s)
{
    CcCharClass_t * c; CcArrayListIter_t iter;

    if (CcCharSet_Elements(s) == 1) {
	self->single = TRUE;
	self->u.chr = CcCharSet_First(s);
	return;
    }
    self->single = FALSE;

    for (c = (CcCharClass_t *)CcArrayList_First(self->classes, &iter);
	 c; c = (CcCharClass_t *)CcArrayList_Next(self->classes, &iter))
	if (CcCharSet_Equals(s, c->set)) {
	    self->u.set = c->set;
	    return;
	}
    c = (CcCharClass_t *)
	CcArrayList_New(self->classes, char_class, "#", CcCharSet_Clone(s));
    self->u.set = c->set;
}

static void
CcAction_AddTarget(CcAction_t * self, CcTarget_t * t)
{
    CcTarget_t * last = NULL;
    CcTarget_t * p = self->target;
    while (p != NULL && t->state->base.index >= p->state->base.index) {
	if (t->state == p->state) return;
	last = p; p = p->next;
    }
    t->next = p;
    if (p == self->target)  self->target = t;
    else  last->next = t;
}

void
CcAction_AddTargets(CcAction_t * self, const CcAction_t * action)
{
    CcTarget_t * p;
    for (p = action->target; p != NULL; p = p->next)
	CcAction_AddTarget(self, CcTarget(p->state));
    if (action->tc == node_contextTrans) self->tc = node_contextTrans;
}
