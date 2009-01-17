/*-------------------------------------------------------------------------
 Author (C) 2009, Charles Wang <charlesw123456@gmail.com>

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
#include  "Indent.h"
#include  "ScanInput.h"

#define  START_INDENT_SPACE 32
CcsBool_t CcsIndent_Init(CcsIndent_t * self, const CcsIndentInfo_t * info)
{
    self->info = info;
    self->lineStart = TRUE;
    if (!(self->indent = CcsMalloc(sizeof(int) * START_INDENT_SPACE)))
	return FALSE;
    self->indentUsed = self->indent;
    self->indentLast = self->indent + START_INDENT_SPACE;
    *self->indentUsed++ = 0;
    self->indentLimit = -1;
    return TRUE;
}
void CcsIndent_Destruct(CcsIndent_t * self)
{
    CcsFree(self->indent);
}

void CcsIndent_SetLimit(CcsIndent_t * self, const CcsToken_t * indentIn)
{
    self->indentLimit = indentIn->loc.col;
}

CcsToken_t *
CcsIndent_Generator(CcsIndent_t * self, CcsScanInput_t * input)
{
    int newLen; int * newIndent, * curIndent;
    CcsToken_t * head, * cur;

    if (!self->lineStart) return NULL;
    CcsAssert(self->indent < self->indentUsed);
    /* Skip blank lines. */
    if (input->ch == '\r' || input->ch == '\n') return NULL;
    /* Dump all required IndentOut when EoF encountered. */
    if (input->ch == EoF) {
	head = NULL;
	while (self->indent < self->indentUsed - 1) {
	    cur = CcsScanInput_NewToken(input, self->info->kIndentOut);
	    cur->next = head; head = cur;
	    --self->indentUsed;
	}
	return head;
    }
    if (self->indentLimit != -1 && input->col >= self->indentLimit) return NULL;
    self->indentLimit = -1;
    self->lineStart = FALSE;
    if (input->col > self->indentUsed[-1]) {
	if (self->indentUsed == self->indentLast) {
	    newLen = (self->indentLast - self->indent) + START_INDENT_SPACE;
	    newIndent = CcsRealloc(self->indent, sizeof(int) * newLen);
	    if (!newIndent) return NULL;
	    self->indentUsed = newIndent + (self->indentUsed - self->indent);
	    self->indentLast = newIndent + newLen;
	    self->indent = newIndent;
	}
	CcsAssert(self->indentUsed < self->indentLast);
	*self->indentUsed++ = input->col;
	return CcsScanInput_NewToken(input, self->info->kIndentIn);
    }
    for (curIndent = self->indentUsed - 1; input->col < *curIndent; --curIndent);
    if (input->col > *curIndent)
	return CcsScanInput_NewToken(input, self->info->kIndentErr);
    head = NULL;
    while (curIndent < self->indentUsed - 1) {
	cur = CcsScanInput_NewToken(input, self->info->kIndentOut);
	cur->next = head; head = cur;
	--self->indentUsed;
    }
    return head;
}
