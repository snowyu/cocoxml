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
#include  "lexical/Action.h"
#include  "lexical/CharClass.h"
#include  "lexical/CharSet.h"
#include  "lexical/Target.h"
#include  "lexical/State.h"

CcAction_t *
CcAction(CcAction_t * self, int typ, int sym, int tc)
{
    self = AllocObject(self, sizeof(CcAction_t));
    self->typ = typ;
    self->sym = sym;
    self->tc = tc;
    self->target = NULL;
    self->next = NULL;
    return self;
}

void
CcAction_Destruct(CcAction_t * self)
{
}

void
CcAction_AddTarget(CcAction_t * self, CcTarget_t * t)
{
    CcTarget_t * last = NULL;
    CcTarget_t * p = self->target;
    while (p != NULL && t->state->nr >= p->state->nr) {
	if (t->state == p->state) return;
	last = p; p = p->next;
    }
    t->next = p;
    if (p == self->target)  self->target = t;
    else  last->next = t;
}

int
CcAction_AddTargets(CcAction_t * self, CcAction_t * a)
{
    CcTarget_t * p, * t;
    for (p = a->target; p != NULL; p = p->next) {
	if (!(t = CcTarget(NULL, p->state))) return -1;
	CcAction_AddTarget(self, t);
    }
    if (a->tc == CcContextTrans) self->tc = CcContextTrans;
    return 0;
}
