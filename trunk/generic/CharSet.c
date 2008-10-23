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
#include  <stdlib.h>
#include  "CharSet.h"

/* Define COCO_WCHAR_MAX here now, it should be placed in Scanner.frame. */
#define   COCO_WCHAR_MAX  65535

struct Range_s {
    int from, to;
    Range_t * next;
};
Range_t *
new_Range(int from, int to)
{
    Range_t * self = malloc(sizeof(Range_t));
    if (self) { self->from = from; self->to = to; self->next = NULL; }
    return self;
}
#define Del_Range(range)  free(range)

CharSet_t *
CharSet(CharSet_t * self)
{
    self->head = NULL;
    return self;
}

void
CharSet_Destruct(CharSet_t * self)
{
    CharSet_Clear(self);
}

gboolean
CharSet_Get(const CharSet_t * self, int i)
{
    Range_t * cur = self->head;
    for (cur = self->head; cur; cur = cur->next) {
	if (i < cur->from) return 0;
	else if (i <= cur->to) return 1;
    }
    return 0;
}

int
CharSet_Set(CharSet_t * self, int i)
{
    Range_t * cur, * prev = NULL, * tmp;

    for (cur = self->head; cur; cur = cur->next) {
	if (i < cur->from - 1) { /* New Range has to be setup. */
	    break;
	} else if (i == cur->from - 1) { /* Extend cur->from is ok. */
	    --cur->from;
	    return 0;
	} else if (i <= cur->to) { /* In cur already. */
	    return 0;
	} else if (i == cur->to + 1) { /* Extend cur->to is ok. */
	    if (cur->next != NULL && i == cur->next->from - 1) {
		/* Combine cur and cur->next. */
		tmp = cur->next;
		cur->to = cur->next->to;
		cur->next = cur->next->next;
		Del_Range(tmp);
	    } else {
		++cur->to;
	    }
	    return 0;
	}
	prev = cur;
    }
    if (!(tmp = new_Range(i, i))) return -1;
    tmp->next = cur;
    if (prev == NULL) self->head = tmp;
    else prev->next = tmp;
    return 0;
}

CharSet_t *
CharSet_Clone(CharSet_t * self, const CharSet_t * s)
{
    Range_t * prev, * curnew;
    const Range_t * cur1;
    self->head = NULL; prev = NULL;
    for (cur1 = s->head; cur1; cur1 = cur1->next) {
	if (!(curnew = new_Range(cur1->from, cur1->to))) {
	    CharSet_Clear(self);
	    return NULL;
	}
	if (prev == NULL) self->head = curnew;
	else prev->next = curnew;
	prev = curnew;
    }
    return self;
}

gboolean
CharSet_Equals(const CharSet_t * self, const CharSet_t * s)
{
    const Range_t * cur0, * cur1;
    cur0 = self->head; cur1 = s->head;
    while (cur0 && cur1) {
	if (cur0->from != cur1->from || cur0->to != cur1->to) return 0;
	cur0 = cur0->next; cur1 = cur1->next;
    }
    return cur0 == cur1;
}

int
CharSet_Elements(const CharSet_t * self)
{
    int cnt = 0;
    const Range_t * cur;
    for (cur = self->head; cur; cur = cur->next)
	cnt += cur->to - cur->from + 1;
    return cnt;
}

int
CharSet_First(const CharSet_t * self)
{
    if (self->head) return self->head->from;
    return -1;
}

int
CharSet_Or(CharSet_t * self, const CharSet_t * s)
{
    Range_t * tmp, * cur0 = self->head, * prev = NULL;
    const Range_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to + 1) {
	    /* cur1 has to be inserted before cur0. */
	    if (!(tmp = new_Range(cur1->from, cur1->to))) return -1;
	    if (prev == NULL) self->head = tmp;
	    else prev->next = tmp;
	    tmp->next = cur0;
	    prev = tmp; cur1 = cur1->next;
	} else if (cur0->to + 1 >= cur1->from) {
	    /* cur0, cur1 overlapped, expand cur0. */
	    if (cur0->from > cur1->from) cur0->from = cur1->from;
	    if (cur0->to < cur1->to) {
		cur0->to = cur1->to;
		/* Try to combine cur0->next. */
		while (cur0->next && cur0->next->from <= cur0->to + 1) {
		    tmp = cur0->next;
		    cur0->next = cur0->next->next;
		    if (cur0->to < tmp->to) cur0->to = tmp->to;
		}
	    }
	    cur1 = cur1->next;
	} else { /* cur0->to + 1 < cur1->from */
	    prev = cur0; cur0 = cur0->next;
	}
    }
    while (cur1) { /* Add all of the remaining. */
	if (!(tmp = new_Range(cur1->from, cur1->to))) return -1;
	if (prev == NULL) self->head = tmp;
	else prev->next = tmp;
	prev = tmp; cur1 = cur1->next;
    }
    return 0;
}

int
CharSet_And(CharSet_t * self, const CharSet_t * s)
{
    Range_t * tmp, * cur0 = self->head, * prev = NULL;
    const Range_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to) {
	    cur1 = cur1->next;
	} else if (cur0->to <= cur1->from) {
	    if (cur0->from < cur1->from) cur0->from = cur1->from;
	    if (cur0->to > cur1->to) {
		if (!(tmp = new_Range(cur1->to + 2, cur0->to))) return -1;
		cur0->to = cur1->to;
		tmp->next = cur0->next; cur0->next = tmp;
		cur1 = cur1->next;
	    }
	    cur0 = cur0->next;
	} else { /* cur0->to > cur1->from, Delete cur0 */
	    tmp = cur0; cur0 = cur0->next;
	    if (prev == NULL) self->head = cur0;
	    else prev->next = cur0;
	    Del_Range(tmp);
	}
    }
    return 0;
}

int
CharSet_Subtract(CharSet_t * self, const CharSet_t * s)
{
    Range_t * tmp, * cur0 = self->head, * prev = NULL;
    const Range_t * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from > cur1->to) {
	    cur1 = cur1->next;
	} else if (cur0->from < cur1->from && cur0->to <= cur1->to) {
	    cur0->to = cur1->from - 1;
	    prev = cur0; cur0 = cur0->next;
	} else if (cur0->from < cur1->from && cur0->to > cur1->to) {
	    if (!(tmp = new_Range(cur1->to + 1, cur0->to))) return -1;
	    tmp->next = cur0->next; cur0->next = tmp;
	    prev = cur0; cur0 = cur0->next;
	    cur1 = cur1->next;
	} else if (cur0->from >= cur1->from && cur0->to <= cur1->to) {
	    tmp = cur0; cur0 = cur0->next;
	    if (prev == NULL) self->head = cur0;
	    else prev->next = cur0;
	    Del_Range(tmp);
	} else if (cur0->from >= cur1->from && cur0->to > cur1->to) {
	    cur0->from = cur1->to + 1;
	    cur1 = cur1->next;
	} else { /* cur0->to < cur1->from */
	    prev = cur0; cur0 = cur0->next;
	}
    }
    return 0;
}

gboolean
CharSet_Includes(const CharSet_t * self, const CharSet_t * s)
{
    const Range_t * cur0 = self->head, * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->to < cur1->from)
	    cur0 = cur0->next;
	else if (cur0->from <= cur1->from && cur0->to >= cur1->to)
	    cur1 = cur1->next;
	else /* cur0->to >= cur1->from &&  */
	    return 0;
    }
    return cur1 == NULL;
}

gboolean
CharSet_Intersects(const CharSet_t * self, const CharSet_t * s)
{
    const Range_t * cur0 = self->head, * cur1 = s->head;

    while (cur0 && cur1) {
	if (cur0->from <= cur1->from && cur1->to >= cur1->from) return 1;
	if (cur0->from <= cur1->to && cur1->to >= cur1->to) return 1;
	if (cur0->from < cur1->from) cur0 = cur0->next;
	else cur1 = cur1->next;
    }
    return 0;
}

void
CharSet_Clear(CharSet_t * self)
{
    Range_t * del;
    while (self->head != NULL) {
	del = self->head;
	self->head = self->head->next;
	free(del);
    }
}

int
CharSet_Fill(CharSet_t * self)
{
    CharSet_Clear(self);
    return (self->head = new_Range(0, COCO_WCHAR_MAX)) ? 0 : -1;
}
