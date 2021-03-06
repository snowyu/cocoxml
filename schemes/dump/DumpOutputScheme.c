/*-------------------------------------------------------------------------
  Copyright (C) 2008, Charles Wang
  Author: Charles Wang <charlesw123456@gmail.com>
  License: GPLv2 (see LICENSE-GPL)
-------------------------------------------------------------------------*/
#include  "dump/DumpOutputScheme.h"
#include  "lexical/State.h"
#include  "lexical/Action.h"
#include  "lexical/Target.h"
#include  "lexical/CharSet.h"

static const char *
CharRepr(char * buf, size_t szbuf, int ch)
{
    if (ch == '\\') {
	snprintf(buf, szbuf, "'\\\\'");
    } else if (ch == '\'') {
	snprintf(buf, szbuf, "'\\\''");
    } else if (ch == '&') {
	snprintf(buf, szbuf, "'&amp;'");
    } else if (ch == '<') {
	snprintf(buf, szbuf, "'&lt;'");
    } else if (ch == '>') {
	snprintf(buf, szbuf, "'&gt;'");
    } else if (ch >= 32 && ch <= 126) {
	snprintf(buf, szbuf, "'%c'", (char)ch);
    } else if (ch == '\a') {
	snprintf(buf, szbuf, "'\\a'");
    } else if (ch == '\b') {
	snprintf(buf, szbuf, "'\\b'");
    } else if (ch == '\f') {
	snprintf(buf, szbuf, "'\\f'");
    } else if (ch == '\n') {
	snprintf(buf, szbuf, "'\\n'");
    } else if (ch == '\r') {
	snprintf(buf, szbuf, "'\\r'");
    } else if (ch == '\t') {
	snprintf(buf, szbuf, "'\\t'");
    } else if (ch == '\v') {
	snprintf(buf, szbuf, "'\\v'");
    } else {
	snprintf(buf, szbuf, "%d", ch);
    }
    return buf;
}

static CcsBool_t
DumpEBNF(CcOutput_t * output, const CcEBNF_t * ebnf)
{
    CcArrayListIter_t iter; const CcNode_t * node;

    for (node = (const CcNode_t *)CcArrayList_FirstC(&ebnf->nodes, &iter);
	 node; node = (const CcNode_t *)CcArrayList_NextC(&ebnf->nodes, &iter))
	CcPrintfIL(output, "<tr><td>%d</td><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%s</td><td>%d</td></tr>",
		   node->base.index,
		   node->base.type->name,
		   node->next ? node->next->base.index : -1,
		   node->down ? node->down->base.index : -1,
		   node->sub ? node->sub->base.index : -1,
		   node->up ? "TRUE" : "FALSE",
		   node->line);
    return TRUE;
}

static CcsBool_t
DOS_LexicalNodes(CcOutputScheme_t * self, CcOutput_t * output)
{
    return DumpEBNF(output, &self->globals->lexical->base);
}

static CcsBool_t
DOS_SyntaxNodes(CcOutputScheme_t * self, CcOutput_t * output)
{
    return DumpEBNF(output, &self->globals->syntax.base);
}

static CcsBool_t
DOS_Terminals(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter; const CcSymbolT_t * sym;
    CcArrayList_t * terminals = &self->globals->symtab.terminals;

    for (sym = (const CcSymbolT_t *)CcArrayList_First(terminals, &iter);
	 sym; sym = (const CcSymbolT_t *)CcArrayList_Next(terminals, &iter))
	CcPrintfIL(output, "<tr><td>%d</td><td>%s</td><td>%s</td></tr>",
		   sym->base.base.index, sym->base.name,
		   sym->tokenKind == symbol_fixedToken ? "fixed" :
		   sym->tokenKind == symbol_classToken ? "class" :
		   sym->tokenKind == symbol_litToken ? "lit" :
		   sym->tokenKind == symbol_classLitToken ? "classLit" :
		   "???");
    return TRUE;
}

static CcsBool_t
DOS_Pragmas(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter; const CcSymbolPR_t * sym;
    CcArrayList_t * pragmas = &self->globals->symtab.pragmas;

    for (sym = (const CcSymbolPR_t *)CcArrayList_First(pragmas, &iter);
	 sym; sym = (const CcSymbolPR_t *)CcArrayList_Next(pragmas, &iter))
	CcPrintfIL(output, "<tr><td>%d</td><td>%s</td><td>%s</td></tr>",
		   sym->base.base.index, sym->base.name,
		   sym->tokenKind == symbol_fixedToken ? "fixed" :
		   sym->tokenKind == symbol_classToken ? "class" :
		   sym->tokenKind == symbol_litToken ? "lit" :
		   sym->tokenKind == symbol_classLitToken ? "classLit" :
		   "???");
    return TRUE;
}

static CcsBool_t
DOS_NonTerminals(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter; const CcSymbolNT_t * sym;
    char * firstStr, * followStr, * ntsStr; int index;
    CcArrayList_t * terminals = &self->globals->symtab.terminals;
    CcArrayList_t * nonterminals = &self->globals->symtab.nonterminals;

    firstStr = CcMalloc(terminals->Count + 1); firstStr[terminals->Count] = 0;
    followStr = CcMalloc(terminals->Count + 1); followStr[terminals->Count] = 0;
    ntsStr = CcMalloc(nonterminals->Count + 1); ntsStr[nonterminals->Count] = 0;
    for (sym = (const CcSymbolNT_t *)CcArrayList_First(nonterminals, &iter);
	 sym; sym = (const CcSymbolNT_t *)CcArrayList_Next(nonterminals, &iter)) {
	CcsAssert(terminals->Count == CcBitArray_getCount(sym->first));
	for (index = 0; index < terminals->Count; ++index)
	    firstStr[index] = CcBitArray_Get(sym->first, index) ? '*' : '.';
	CcsAssert(terminals->Count == CcBitArray_getCount(sym->follow));
	for (index = 0; index < terminals->Count; ++index)
	    followStr[index] = CcBitArray_Get(sym->follow, index) ? '*' : '.';
	CcsAssert(nonterminals->Count == CcBitArray_getCount(sym->nts));
	for (index = 0; index < nonterminals->Count; ++index)
	    ntsStr[index] = CcBitArray_Get(sym->nts, index) ? '*' : '.';
	CcPrintfIL(output, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>",
		   sym->base.base.index, sym->base.name,
		   sym->deletable ? "TRUE" : "FALSE",
		   sym->graph->base.index,
		   firstStr, followStr, ntsStr);
    }
    CcFree(ntsStr); CcFree(followStr); CcFree(firstStr);
    return TRUE;
}

static CcsBool_t
DOS_States(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter;
    const CcState_t * state;
    const CcAction_t * action;
    const CcTarget_t * target;
    CcCharSet_t * s;
    const CcRange_t * curRange;
    char buf0[8], buf1[8];
    const CcArrayList_t * states = &self->globals->lexical->states;

    for (state = (const CcState_t *)CcArrayList_FirstC(states, &iter);
	 state; state = (const CcState_t *)CcArrayList_NextC(states, &iter)) {
	CcPrintfIL(output, "<tr><td>%d</td><td>%s</td><td>%s</td></tr>",
		   state->base.index,
		   state->endOf ? state->endOf->name : "(null)",
		   state->ctx ? "TRUE" : "FALSE");
	for (action = state->firstAction; action; action = action->next) {
	    CcPrintfI(output, "<tr><td></td><td>");
	    s = CcAction_GetShift(action);
	    for (curRange = s->head; curRange; curRange = curRange->next) {
		if (curRange->from == curRange->to) {
		    CcPrintf(output, "%s",
			     CharRepr(buf0, sizeof(buf0), curRange->from));
		} else {
		    CcPrintf(output, "[%s, %s]",
			     CharRepr(buf0, sizeof(buf0), curRange->from),
			     CharRepr(buf1, sizeof(buf1), curRange->to));
		}
		if (curRange->next) CcPrintf(output, "&nbsp");
	    }
	    CcCharSet_Destruct(s);
	    CcPrintf(output, "</td><td>");
	    for (target = action->target; target; target = target->next) {
		CcPrintf(output, "%d", target->state->base.index);
		if (target->next) CcPrintf(output, ",");
	    }
	    CcPrintfL(output, "</td></tr>");
	}
    }
    return TRUE;
}

static CcsBool_t
CcDumpOutputScheme_write(CcOutputScheme_t * self, CcOutput_t * output,
			 const char * func, const char * param)
{
    if (!strcmp(func, "lexicalNodes")) {
	return DOS_LexicalNodes(self, output);
    } else if (!strcmp(func, "syntaxNodes")) {
	return DOS_SyntaxNodes(self, output);
    } else if (!strcmp(func, "terminals")) {
	return DOS_Terminals(self, output);
    } else if (!strcmp(func, "pragmas")) {
	return DOS_Pragmas(self, output);
    } else if (!strcmp(func, "nonterminals")) {
	return DOS_NonTerminals(self, output);
    } else if (!strcmp(func, "states")) {
	return DOS_States(self, output);
    }
    fprintf(stderr, "Unknown section '%s' encountered.\n", func);
    return FALSE;
}

static void
CcDumpOutputScheme_Destruct(CcObject_t * self)
{
    CcOutputScheme_Destruct(self);
}

static const CcOutputSchemeType_t DumpOutputSchemeType = {
    { sizeof(CcDumpOutputScheme_t), "DumpOutputScheme",
      CcDumpOutputScheme_Destruct },
    /* If the following lists are modified, modify install.py too. */
    "NodeTable.html\0StateTable.html\0SymbolTable.html\0\0",
    CcDumpOutputScheme_write
};

CcDumpOutputScheme_t *
CcDumpOutputScheme(CcsParser_t * parser, CcsXmlParser_t * xmlparser,
		   CcArguments_t * arguments)
{
    CcDumpOutputScheme_t * self = (CcDumpOutputScheme_t *)
	CcOutputScheme(&DumpOutputSchemeType,
		       parser ? &parser->globals :
		       xmlparser ? &xmlparser->globals : NULL, arguments);
    return self;
}
