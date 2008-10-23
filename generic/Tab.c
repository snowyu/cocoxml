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
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  "Tab.h"
#include  "Symbol.h"

Tab_t *
Tab(Tab_t * self, Parser_t * parser) {
    if (!ArrayList(&self->terminals)) goto errquit0;
    if (!ArrayList(&self->pragmas)) goto errquit1;
    if (!ArrayList(&self->nonterminals)) goto errquit2;
    return self;
#if 0
 errquit3:
    ArrayList_Destruct(&self->nonterminals);
#endif
 errquit2:
    ArrayList_Destruct(&self->pragmas);
 errquit1:
    ArrayList_Destruct(&self->terminals);
 errquit0:
    return NULL;
}

Symbol_t *
Tab_NewSym(Tab_t * self, int typ, const char * name, int line)
{
    Symbol_t * sym;

#if 0  /* Is this possible */
    if (!strcmp(name, "\"\"")) {
	self->parser->SemErr("empty token now allowed");
	name = "???";
    }
#endif
    if (!(sym = malloc(sizeof(Symbol_t)))) return NULL;
    if (!(Symbol(sym, typ, name, line))) { free(sym); return NULL; }

    if (typ == node_t) {
	sym->n = self->terminals.Count;
	ArrayList_Add(&self->terminals, sym);
    } else if (typ == node_pr) {
	ArrayList_Add(&self->pragmas, sym);
    } else if (typ == node_nt) {
	sym->n = self->nonterminals.Count;
	ArrayList_Add(&self->nonterminals, sym);
    }
    return sym;
}

Symbol_t *
Tab_FindSym(Tab_t * self, const char * name)
{
    int idx; Symbol_t * sym;
    for (idx = 0; idx < self->terminals.Count; ++idx) {
	sym = (Symbol_t *)ArrayList_Get(&self->terminals, idx);
	if (!strcmp(sym->name, name)) return sym;
    }
    for (idx = 0; idx < self->nonterminals.Count; ++idx) {
	sym = (Symbol_t *)ArrayList_Get(&self->nonterminals, idx);
	if (!strcmp(sym->name, name)) return sym;
    }
    return NULL;
}

void
Tab_PrintSymbolTable(Tab_t * self)
{
    int idx; Symbol_t * sym; HTIterator_t iter;
    DumpBuffer_t dbuf; char buf[128];

    fprintf(self->trace, "Symbol Table:\n");
    fprintf(self->trace, "------------\n\n");
    fprintf(self->trace, " nr name          typ  hasAt graph  del    line tokenKind\n");

    for (idx = 0; idx < self->terminals.Count; ++idx) {
	sym = (Symbol_t *)ArrayList_Get(&self->terminals, idx);
	DumpBuffer(&dbuf, buf, sizeof(buf));
	Symbol_Dump(sym, &dbuf);
	fprintf(self->trace, "%s\n", buf);
    }

    for (idx = 0; idx < self->pragmas.Count; ++idx) {
	sym = (Symbol_t *)ArrayList_Get(&self->pragmas, idx);
	DumpBuffer(&dbuf, buf, sizeof(buf));
	Symbol_Dump(sym, &dbuf);
	fprintf(self->trace, "%s\n", buf);
    }

    for (idx = 0; idx < self->nonterminals.Count; ++idx) {
	sym = (Symbol_t *)ArrayList_Get(&self->nonterminals, idx);
	DumpBuffer(&dbuf, buf, sizeof(buf));
	Symbol_Dump(sym, &dbuf);
	fprintf(self->trace, "%s\n", buf);
    }

    fprintf(self->trace, "\nLiteral Tokens:\n");
    fprintf(self->trace, "--------------\n");

    HashTable_GetIterator(&self->literals, &iter);
    while (HTIterator_Forward(&iter)) {
	fprintf(self->trace, "_%s =  %s.\n",
		((Symbol_t *)HTIterator_Value(&iter))->name,
		HTIterator_Key(&iter));
    }
    fprintf(self->trace, "\n");
}
