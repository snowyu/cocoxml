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
#include  "State.h"
#include  "Action.h"

CcState_t *
CcState(CcState_t * self)
{
    self->nr = 0;
    self->firstAction = NULL;
    self->endOf = NULL;
    self->ctx = 0;
    self->next = NULL;
    return self;
}

void
CcState_Destruct(CcState_t * self)
{
}

void
CcState_AddAction(CcState_t * self, CcAction_t * act)
{
    CcAction_t * lasta = NULL, * a = self->firstAction;
    while (a != NULL && act->typ >= a->typ) { lasta = a; a = a->next; }
    act->next = a;
    if (a == self->firstAction) self->firstAction = act;
    else  lasta->next = act;
}

void
CcState_DetachAction(CcState_t * self, CcAction_t * act)
{
    CcAction_t * lasta = NULL, * a = self->firstAction;
    while (a != NULL && a != act) { lasta = a; a = a->next; }
    if (a != NULL) {
	if (a == self->firstAction) self->firstAction = a->next;
	else lasta->next = a->next;
    }
}

int
CcState_MeltWith(CcState_t * self, CcState_t * s)
{
    CcAction_t * action, * a;
    for (action = s->firstAction; action != NULL; action = action->next) {
	if (!(a = CcAction(NULL, action->typ, action->sym, action->tc)))
	    return -1;
	CcAction_AddTargets(a, action);
	CcState_AddAction(self, a);
    }
    return 0;
}
