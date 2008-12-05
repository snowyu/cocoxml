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
#include  "c/COutputScheme.h"
#include  "lexical/Action.h"
#include  "lexical/CharSet.h"
#include  "lexical/Comment.h"
#include  "lexical/State.h"
#include  "lexical/Target.h"
#include  "lexical/Transition.h"
#include  "XmlSpec.h"
#include  "syntax/Nodes.h"

/* When the number of possible terminals is greater than maxTerm,
   symSet is used. */
#define  maxTerm  3

static const char *
CharRepr(char * buf, size_t szbuf, int ch)
{
    if (ch == '\\') {
	snprintf(buf, szbuf, "'\\\\'");
    } else if (ch == '\'') {
	snprintf(buf, szbuf, "'\\\''");
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
COS_Defines(CcOutputScheme_t * self, CcOutput_t * output)
{
    if (!self->globals->lexical->ignoreCase)
	CcPrintfI(output, "#define CASE_SENSITIVE\n");
    return TRUE;
}

static CcsBool_t
COS_Declarations(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcPrintfI(output, "self->eofSym = %d;\n",
	      self->globals->syntax.eofSy->kind);
    CcPrintfI(output, "self->maxT = %d;\n",
	      self->globals->symtab.terminals.Count - 1);
    CcPrintfI(output, "self->noSym = %d;\n",
	      self->globals->syntax.noSy->kind);
    return TRUE;
}

static CcsBool_t
COS_Chars2States(CcOutputScheme_t * self, CcOutput_t * output)
{
    int numEle;
    CcLexical_StartTab_t * table, * cur;
    char buf0[8], buf1[8];
    CcPrintfI(output, "{ EoF, EoF, -1 },\n");
    table = CcLexical_GetStartTab(self->globals->lexical, &numEle);
    for (cur = table; cur - table < numEle; ++cur)
	CcPrintfI(output, "{ %d, %d, %d },\t/* %s %s */\n",
		  cur->keyFrom, cur->keyTo, cur->state,
		  CharRepr(buf0, sizeof(buf0), cur->keyFrom),
		  CharRepr(buf1, sizeof(buf1), cur->keyTo));
    CcFree(table);
    return TRUE;
}

static CcsBool_t
COS_Identifiers2KeywordKinds(CcOutputScheme_t * self, CcOutput_t * output)
{
    int numEle;
    CcLexical_Identifier_t * list, * cur;

    list = CcLexical_GetIdentifiers(self->globals->lexical, &numEle);
    for (cur = list; cur - list < numEle; ++cur)
	CcPrintfI(output, "{ %s, %d },\n", cur->name, cur->index);
    CcLexical_Identifiers_Destruct(list, numEle);
    return TRUE;
}

static CcsBool_t
COS_Comments(CcOutputScheme_t * self, CcOutput_t * output)
{
    const CcComment_t * cur;
    char buf0[8], buf1[8], buf2[8], buf3[8];
    output->indent += 4;
    for (cur = self->globals->lexical->firstComment; cur; cur = cur->next)
	CcPrintfI(output, "{ { %s, %s }, { %s, %s }, %s },\n",
		  CharRepr(buf0, sizeof(buf0), cur->start[0]),
		  CharRepr(buf1, sizeof(buf1), cur->start[1]),
		  CharRepr(buf2, sizeof(buf2), cur->stop[0]),
		  CharRepr(buf3, sizeof(buf3), cur->stop[1]),
		  cur->nested ? "TRUE" : "FALSE");
    output->indent -= 4;
    return TRUE;
}

static CcsBool_t
COS_Scan1(CcOutputScheme_t * self, CcOutput_t * output)
{
    const CcRange_t * curRange;
    char buf0[8], buf1[8];
    for (curRange = self->globals->lexical->ignored->head;
	 curRange; curRange = curRange->next) {
	if (curRange->from == curRange->to)
	    CcPrintfI(output, "|| self->ch == %s\n",
		      CharRepr(buf0 ,sizeof(buf0), curRange->from));
	else
	    CcPrintfI(output, "|| (self->ch >= %s && self->ch <= %s)\n",
		      CharRepr(buf0 ,sizeof(buf0), curRange->from),
		      CharRepr(buf1 ,sizeof(buf1), curRange->to));
    }
    return TRUE;
}

static void
COS_WriteState(CcOutputScheme_t * self, CcOutput_t * output,
	       const CcState_t * state, const CcBitArray_t * mask)
{
    const CcAction_t * action;
    CcCharSet_t * s; const CcRange_t * curRange;
    char buf0[8], buf1[8];
    int sIndex = state->base.index;

    if (CcBitArray_Get(mask, sIndex))
	CcPrintfI(output, "case %d: case_%d:\n", sIndex, sIndex);
    else
	CcPrintfI(output, "case %d:\n", sIndex);
    output->indent += 4;
    for (action = state->firstAction; action != NULL; action = action->next) {
	if (action == state->firstAction) CcPrintfI(output, "if (");
	else CcPrintfI(output, "} else if (");
	s = CcTransition_GetCharSet(&action->trans);
	for (curRange = s->head; curRange; curRange = curRange->next) {
	    if (curRange != s->head) CcPrintfI(output, "    ");
	    if (curRange->from == curRange->to)
		CcPrintf(output,"self->ch == %s",
			CharRepr(buf0, sizeof(buf0), curRange->from));
	    else
		CcPrintf(output, "(self->ch >= %s && self->ch <= %s)",
			 CharRepr(buf0, sizeof(buf0), curRange->from),
			 CharRepr(buf1, sizeof(buf1), curRange->to));
	    if (curRange->next) CcPrintf(output, " ||\n");
	}
	CcPrintf(output, ") {\n");
	output->indent += 4;
	CcPrintfI(output, "CcsScanner_GetCh(self); goto case_%d;\n",
		  action->target->state->base.index);
	output->indent -= 4;
	CcCharSet_Destruct(s);
    }

    if (state->firstAction == NULL) CcPrintfI(output, "{ ");
    else CcPrintfI(output, "} else { ");
    if (state->endOf == NULL) {
	CcPrintf(output, "kind = self->noSym;");
    } else if (CcSymbol_GetTokenKind(state->endOf) != symbol_classLitToken) {
	CcPrintf(output, "kind = %d;", state->endOf->kind);
    } else {
	CcPrintf(output,
		 "kind = CcsScanner_GetKWKind(self, pos, self->pos, %d);",
		 state->endOf->kind);
    }
    CcPrintf(output, " break; }\n");
    output->indent -= 4;
}

static CcsBool_t
COS_Scan3(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter;
    CcBitArray_t mask;
    const CcState_t * state;
    CcArrayList_t * stateArr = &self->globals->lexical->states;

    CcLexical_TargetStates(self->globals->lexical, &mask);
    state = (CcState_t *)CcArrayList_First(stateArr, &iter);
    for (state = (const CcState_t *)CcArrayList_Next(stateArr, &iter);
	 state; state = (const CcState_t *)CcArrayList_Next(stateArr, &iter))
	COS_WriteState(self, output, state, &mask);
    CcBitArray_Destruct(&mask);
    return TRUE;
}

static CcsBool_t
COS_KindUnknownNS(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcsAssert(self->globals->xmlspecmap);
    CcPrintfI(output, "int kindUnknownNS = %d;\n", self->globals->xmlspecmap);
    return TRUE;
}

static int
cmpSpecKey(const void * cs0, const void * cs1)
{
    return strcmp(*(const char **)cs0, *(const char **)cs1);
}

static CcsBool_t
COS_XmlSpecSubList(CcOutputScheme_t * self, CcOutput_t * output)
{
    int count; CcHTIterator_t iter;
    const char ** keylist, ** curkey;
    const CcXmlSpec_t * spec;
    CcXmlSpecData_t * datalist, * datacur; size_t datanum; char * tmp;
    CcXmlSpecMap_t * map = self->globals->xmlspecmap;

    CcsAssert(map != NULL);
    count = CcHashTable_Num(&map->map);
    keylist = curkey = CcMalloc(sizeof(char *) * count);
    CcHashTable_GetIterator(&map->map, &iter);
    while (CcHTIterator_Forward(&iter)) *curkey++ = CcHTIterator_Key(&iter);
    CcsAssert(curkey - keylist == count);
    qsort(keylist, count, sizeof(const char *), cmpSpecKey);

    CcPrintfI(output, "static const CcsXmlTag_t XmlTags[] = {\n");
    for (curkey = keylist; curkey - keylist < count; ++curkey) {
	spec = (const CcXmlSpec_t *)CcHashTable_Get(&map->map, *curkey);
	CcsAssert(spec != NULL);
	datalist = CcXmlSpec_GetSortedTagList(spec, self->globals, &datanum);
	if (datanum == 0) continue;
	output->indent += 4;
	for (datacur = datalist; datacur - datalist < datanum; ++datacur) {
	    tmp = CcEscape(datacur->name);
	    CcPrintfI(output, "{ %s, %d, %d },\n",
		      tmp, datacur->kind0, datacur->kind1);
	    CcFree(tmp);
	}
	CcXmlSpecData_Destruct(datalist, datanum);
    }
    CcPrintf(output, "};\n");

    CcPrintfI(output, "static const CcsXmlAttr_t XmlAttrs[] = {\n");
    for (curkey = keylist; curkey - keylist < count; ++curkey) {
	spec = (const CcXmlSpec_t *)CcHashTable_Get(&map->map, *curkey);
	CcsAssert(spec != NULL);
	datalist = CcXmlSpec_GetSortedAttrList(spec, self->globals, &datanum);
	if (datanum == 0) continue;
	output->indent += 4;
	for (datacur = datalist; datacur - datalist < datanum; ++datacur) {
	    tmp = CcEscape(datacur->name);
	    CcPrintfI(output, "{ %s, %d },\n", tmp, datacur->kind0);
	    CcFree(tmp);
	}
	CcXmlSpecData_Destruct(datalist, datanum);
    }
    CcPrintf(output, "};\n");

    CcPrintfI(output, "static const CcsXmlPInstruction_t XmlPIs[] = {\n");
    for (curkey = keylist; curkey - keylist < count; ++curkey) {
	spec = (const CcXmlSpec_t *)CcHashTable_Get(&map->map, *curkey);
	CcsAssert(spec != NULL);
	datalist = CcXmlSpec_GetSortedPIList(spec, self->globals, &datanum);
	if (datanum == 0) continue;
	output->indent += 4;
	for (datacur = datalist; datacur - datalist < datanum; ++datacur) {
	    tmp = CcEscape(datacur->name);
	    CcPrintfI(output, "{ %s, %d },\n", tmp, datacur->kind0);
	    CcFree(tmp);
	}
	CcXmlSpecData_Destruct(datalist, datanum);
    }
    CcPrintf(output, "};\n");

    CcFree(keylist);
    return TRUE;
}

static CcsBool_t
COS_XmlSpecList(CcOutputScheme_t * self, CcOutput_t * output)
{
    int count; CcHTIterator_t iter;
    const char ** keylist, ** curkey;
    int cntTagList, cntAttrList, cntPIList;
    char * tmp; CcsXmlSpecOption_t option;
    const CcXmlSpec_t * spec;
    CcXmlSpecData_t * datalist; size_t datanum;
    CcXmlSpecMap_t * map = self->globals->xmlspecmap;

    CcsAssert(map != NULL);
    count = CcHashTable_Num(&map->map);
    keylist = curkey = CcMalloc(sizeof(char *) * count);
    CcHashTable_GetIterator(&map->map, &iter);
    while (CcHTIterator_Forward(&iter)) *curkey++ = CcHTIterator_Key(&iter);
    CcsAssert(curkey - keylist == count);
    qsort(keylist, count, sizeof(const char *), cmpSpecKey);

    cntTagList = cntAttrList = cntPIList = 0;
    for (curkey = keylist; curkey - keylist < count; ++curkey) {
	spec = (const CcXmlSpec_t *)CcHashTable_Get(&map->map, *curkey);
	CcsAssert(spec != NULL);
	tmp = CcEscape(*curkey);
	CcPrintfI(output, "{ %s, %s,\n", tmp,
		  spec->caseSensitive ? "TRUE" : "FALSE");
	CcFree(tmp);
	output->indent += 4;
	CcPrintfI(output, "{");
	for (option = XSO_UnknownTag; option < XSO_SIZE; ++option)
	    CcPrintf(output, " %d,", spec->options[option]);
	CcPrintf(output, " },\n");

	datalist = CcXmlSpec_GetSortedTagList(spec, self->globals, &datanum);
	if (datanum == 0) CcPrintfI(output, "NULL, 0, /* Tags */\n");
	else {
	    CcPrintfI(output, "XmlTags + %d, %d,\n", cntTagList, datanum);
	    cntTagList += datanum;
	}
	CcXmlSpecData_Destruct(datalist, datanum);

	datalist = CcXmlSpec_GetSortedAttrList(spec, self->globals, &datanum);
	if (datanum == 0) CcPrintfI(output, "NULL, 0, /* Attrs */\n");
	else {
	    CcPrintfI(output, "XmlAttrs + %d, %d,\n", cntAttrList, datanum);
	    cntAttrList += datanum;
	}
	CcXmlSpecData_Destruct(datalist, datanum);

	datalist = CcXmlSpec_GetSortedPIList(spec, self->globals, &datanum);
	if (datanum == 0) {
	    CcPrintfI(output, "NULL, 0, /* Processing Instructions */\n");
	} else {
	    CcPrintfI(output, "XmlPIs + %d, %d,\n", cntPIList, datanum);
	    cntPIList += datanum;
	}
	CcXmlSpecData_Destruct(datalist, datanum);

	output->indent -= 4;
	CcPrintfI(output, "},\n");
    }
    CcFree(keylist);
    return TRUE;
}

static CcsBool_t
COS_Members(CcOutputScheme_t * self, CcOutput_t * output)
{
    if (self->globals->base.parser && self->globals->base.parser->members)
	CcSource(output, self->globals->base.parser->members);
    if (self->globals->base.xmlparser && self->globals->base.xmlparser->members)
	CcSource(output, self->globals->base.xmlparser->members);
    return TRUE;
}

static CcsBool_t
COS_Constructor(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcPrintfI(output, "self->maxT = %d;\n",
	      self->globals->symtab.terminals.Count - 1);
    if (self->globals->base.parser && self->globals->base.parser->constructor)
	CcSource(output, self->globals->base.parser->constructor);
    if (self->globals->base.xmlparser &&
	self->globals->base.xmlparser->constructor)
	CcSource(output, self->globals->base.xmlparser->constructor);
    return TRUE;
}

static CcsBool_t
COS_Destructor(CcOutputScheme_t * self, CcOutput_t * output)
{
    if (self->globals->base.parser && self->globals->base.parser->destructor)
	CcSource(output, self->globals->base.parser->destructor);
    if (self->globals->base.xmlparser &&
	self->globals->base.xmlparser->destructor)
	CcSource(output, self->globals->base.xmlparser->destructor);
    return TRUE;
}

static CcsBool_t
COS_Pragmas(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter;
    const CcSymbolPR_t * sym, * sym1;
    const CcArrayList_t * pragmas = &self->globals->symtab.pragmas;

    for (sym = sym1 = (const CcSymbolPR_t *)CcArrayList_FirstC(pragmas, &iter);
	 sym; sym = (const CcSymbolPR_t *)CcArrayList_NextC(pragmas, &iter)) {
	CcPrintfI(output, "%sif (self->la->kind == %d) {\n",
		  (sym == sym1) ? "" : "} else ", sym->base.kind);
	if (sym->semPos) {
	    output->indent += 4;
	    CcSource(output, sym->semPos);
	    output->indent -= 4;
	}
    }
    if (sym1) CcPrintfI(output, "}\n");
    return TRUE;
}

static CcsBool_t
COS_ProductionsHeader(CcOutputScheme_t * self,
		      CcOutput_t * output)
{
    CcArrayListIter_t iter;
    const CcSymbolNT_t * sym;
    const CcArrayList_t * nonterminals = &self->globals->symtab.nonterminals;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    for (sym = (const CcSymbolNT_t *)CcArrayList_FirstC(nonterminals, &iter);
	 sym;
	 sym = (const CcSymbolNT_t *)CcArrayList_NextC(nonterminals, &iter))
	if (sym->attrPos)
	    CcPrintfI(output,
		      "static void %s_%s(%s_t * self, %s);\n",
		      ccself->ParserStem, sym->base.name,
		      ccself->ParserStem, sym->attrPos->text);
	else
	    CcPrintfI(output, "static void %s_%s(%s_t * self);\n",
		      ccself->ParserStem, sym->base.name, ccself->ParserStem);
    return TRUE;
}

static CcsBool_t
COS_ParseRoot(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;
    CcPrintfI(output, "%s_%s(self);\n", ccself->ParserStem,
	      self->globals->syntax.gramSy->name);
    return TRUE;
}

static void
SCOS_GenCond(CcOutputScheme_t * self, CcOutput_t * output,
	     const char * prefix, const char * suffix,
	     const CcBitArray_t * s, const CcNode_t * p)
{
    const CcNodeRSLV_t * prslv; int n;
    CcArrayListIter_t iter; const CcSymbol_t * sym;
    const CcArrayList_t * terminals;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    if (p->base.type == node_rslv) {
	prslv = (CcNodeRSLV_t *)p;
	CcPrintfI(output, "%s%s%s\n", prefix, prslv->pos->text, suffix);
    } else if ((n = CcBitArray_Elements(s)) == 0) {
	CcPrintfI(output, "%s%s%s\n", prefix, "FALSE", suffix);
    } else if (n <= maxTerm) {
	CcPrintfI(output, "%s", prefix);
	terminals = &self->globals->symtab.terminals;
	for (sym = (const CcSymbol_t *)CcArrayList_FirstC(terminals, &iter);
	     sym; sym = (const CcSymbol_t *)CcArrayList_NextC(terminals, &iter))
	    if (CcBitArray_Get(s, sym->kind)) {
		CcPrintf(output, "self->la->kind == %d", sym->kind);
		if (--n > 0) CcPrintf(output, " || ");
	    }
	CcPrintf(output, "%s\n", suffix);
    } else {
	CcPrintfI(output, "%s%s_StartOf(self, %d)%s\n",
		  prefix, ccself->ParserStem,
		  CcSyntaxSymSet_New(&ccself->symSet, s), suffix);
    }
}

static CcsBool_t
SCOS_UseSwitch(CcOutputScheme_t * self, CcNode_t * p)
{
    CcBitArray_t s1, s2; int nAlts;
    CcSyntax_t * syntax = &self->globals->syntax;
    CcArrayList_t * terminals = &self->globals->symtab.terminals;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    if (p->base.type != node_alt) return FALSE;
    nAlts = 0;
    CcBitArray(&s1, terminals->Count);
    while (p != NULL) {
	CcSyntax_Expected0(syntax, &s2, p->sub, ccself->curSy);
	if (CcBitArray_Intersect(&s1, &s2)) goto falsequit2;
	CcBitArray_Or(&s1, &s2);
	CcBitArray_Destruct(&s2);
	++nAlts;
	if (p->sub->base.type == node_rslv) goto falsequit1;
	p = p->down;
    }
    CcBitArray_Destruct(&s1);
    return nAlts > 5;
 falsequit2:
    CcBitArray_Destruct(&s2);
 falsequit1:
    CcBitArray_Destruct(&s1);
    return FALSE;
}

static void
SCOS_GenCode(CcOutputScheme_t * self, CcOutput_t * output,
	     CcNode_t * p, const CcBitArray_t * IsChecked)
{
    int err; CcsBool_t equal, useSwitch; int index;
    CcNode_t * p2; CcBitArray_t s1, s2, isChecked;
    CcNodeNT_t * pnt; CcNodeT_t * pt; CcNodeWT_t * pwt;
    CcNodeSEM_t * psem; CcNodeSYNC_t * psync;
    CcSyntax_t * syntax = &self->globals->syntax;
    CcArrayList_t * terminals = &self->globals->symtab.terminals;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    CcBitArray_Clone(&isChecked, IsChecked);
    while (p != NULL) {
	if (p->base.type == node_nt) {
	    pnt = (CcNodeNT_t *)p;
	    if (pnt->pos) {
		CcPrintfI(output, "%s_%s(self, %s);\n",
			  ccself->ParserStem, pnt->sym->name, pnt->pos->text);
	    } else {
		CcPrintfI(output, "%s_%s(self);\n",
			  ccself->ParserStem, pnt->sym->name);
	    }
	} else if (p->base.type == node_t) {
	    pt = (CcNodeT_t *)p;
	    if (CcBitArray_Get(&isChecked, pt->sym->kind))
		CcPrintfI(output, "%s_Get(self);\n", ccself->ParserStem);
	    else
		CcPrintfI(output, "%s_Expect(self, %d);\n",
			  ccself->ParserStem, pt->sym->kind);
	} else if (p->base.type == node_wt) {
	    pwt = (CcNodeWT_t *)p;
	    CcSyntax_Expected(syntax, &s1, p->next, ccself->curSy);
	    CcBitArray_Or(&s1, syntax->allSyncSets);
	    CcPrintfI(output, "%s_ExpectWeak(self, %d, %d);\n",
		      ccself->ParserStem, pwt->sym->kind,
		      CcSyntaxSymSet_New(&ccself->symSet, &s1));
	    CcBitArray_Destruct(&s1);
	} else if (p->base.type == node_any) {
	    CcPrintfI(output, "%s_Get(self);\n", ccself->ParserStem);
	} else if (p->base.type == node_eps) {
	} else if (p->base.type == node_rslv) {
	} else if (p->base.type == node_sem) {
	    psem = (CcNodeSEM_t *)p;
	    CcSource(output, psem->pos);
	} else if (p->base.type == node_sync) {
	    psync = (CcNodeSYNC_t *)p;
	    err = CcSyntax_SyncError(syntax, ccself->curSy);
	    CcBitArray_Clone(&s1, psync->set);
	    SCOS_GenCond(self, output, "while (!(", ")) {", &s1, p);
	    output->indent += 4;
	    CcPrintfI(output, "%s_SynErr(self, %d); %s_Get(self);\n",
		      ccself->ParserStem, err, ccself->ParserStem);
	    output->indent -= 4;
	    CcPrintfI(output, "}\n");
	    CcBitArray_Destruct(&s1);
	} else if (p->base.type == node_alt) {
	    CcSyntax_First(syntax, &s1, p);
	    equal = CcBitArray_Equal(&s1, &isChecked);
	    CcBitArray_Destruct(&s1);
	    useSwitch = SCOS_UseSwitch(self, p);
	    if (useSwitch)
		CcPrintfI(output, "switch (self->la->kind) {\n");
	    p2 = p;
	    while (p2 != NULL) {
		CcSyntax_Expected(syntax, &s1, p2->sub, ccself->curSy);
		if (useSwitch) {
		    CcPrintfI(output, "");
		    for (index = 0; index < terminals->Count; ++index)
			if (CcBitArray_Get(&s1, index))
			    CcPrintf(output, "case %d: ", index);
		    CcPrintf(output,"{\n");
		} else if (p2 == p) {
		    SCOS_GenCond(self, output, "if (", ") {", &s1, p2->sub);
		} else if (p2->down == NULL && equal) {
		    CcPrintfI(output, "} else {\n");
		} else {
		    SCOS_GenCond(self, output,
				 "} else if (", ") {", &s1, p2->sub);
		}
		CcBitArray_Or(&s1, &isChecked);
		output->indent += 4;
		SCOS_GenCode(self, output, p2->sub, &s1);
		if (useSwitch) CcPrintfI(output, "break;\n");
		output->indent -= 4;
		if (useSwitch) CcPrintfI(output, "}\n");
		p2 = p2->down;
		CcBitArray_Destruct(&s1);
	    }
	    if (equal) {
		CcPrintfI(output, "}\n");
	    } else {
		err = CcSyntax_AltError(syntax, ccself->curSy);
		if (useSwitch) {
		    CcPrintfI(output, "default: %s_SynErr(self, %d); break;\n",
			      ccself->ParserStem, err);
		    CcPrintfI(output, "}\n");
		} else {
		    CcPrintfI(output, "} else %s_SynErr(self, %d);\n",
			      ccself->ParserStem, err);
		}
	    }
	} else if (p->base.type == node_iter) {
	    p2 = p->sub;
	    if (p2->base.type == node_wt) {
		CcSyntax_Expected(syntax, &s1, p2->next, ccself->curSy);
		CcSyntax_Expected(syntax, &s2, p->next, ccself->curSy);
		CcPrintfI(output,
			  "while (%s_WeakSeparator(self, %d, %d, %d)) {\n",
			  ccself->ParserStem, ((CcNodeWT_t *)p2)->sym->kind,
			  CcSyntaxSymSet_New(&ccself->symSet, &s1),
			  CcSyntaxSymSet_New(&ccself->symSet, &s2));
		CcBitArray_Destruct(&s1); CcBitArray_Destruct(&s2);
		CcBitArray(&s1, terminals->Count);
		if (p2->up || p2->next == NULL) p2 = NULL; else p2 = p2->next;
	    } else {
		CcSyntax_First(syntax, &s1, p2);
		SCOS_GenCond(self, output, "while (", ") {", &s1, p2);
	    }
	    output->indent += 4;
	    SCOS_GenCode(self, output, p2, &s1);
	    output->indent -= 4;
	    CcPrintfI(output, "}\n");
	    CcBitArray_Destruct(&s1);
	} else if (p->base.type == node_opt) {
	    CcSyntax_First(syntax, &s1, p->sub);
	    SCOS_GenCond(self, output, "if (", ") {", &s1, p->sub);
	    output->indent += 4;
	    SCOS_GenCode(self, output, p->sub, &s1);
	    output->indent -= 4;
	    CcPrintfI(output, "}\n");
	    CcBitArray_Destruct(&s1);
	}
	if (p->base.type != node_eps && p->base.type != node_sem &&
	    p->base.type != node_sync)
	    CcBitArray_SetAll(&isChecked, FALSE);
	if (p->up) break;
	p = p->next;
    }
    CcBitArray_Destruct(&isChecked);
}

static CcsBool_t
COS_ProductionsBody(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcBitArray_t isChecked;
    CcArrayListIter_t iter;
    const CcSymbolNT_t * sym;
    const CcArrayList_t * nonterminals = &self->globals->symtab.nonterminals;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    CcBitArray(&isChecked, self->globals->symtab.terminals.Count);
    for (sym = (const CcSymbolNT_t *)CcArrayList_FirstC(nonterminals, &iter);
	 sym;
	 sym = (const CcSymbolNT_t *)CcArrayList_NextC(nonterminals, &iter)) {
	ccself->curSy = (const CcSymbol_t *)sym;
	if (sym->attrPos == NULL) {
	    CcPrintfI(output, "static void\n");
	    CcPrintfI(output, "%s_%s(%s_t * self)\n",
		      ccself->ParserStem, sym->base.name,
		      ccself->ParserStem);
	} else {
	    CcPrintfI(output, "static void\n");
	    CcPrintfI(output, "%s_%s(%s_t * self, %s)\n",
		      ccself->ParserStem, sym->base.name,
		      ccself->ParserStem, sym->attrPos->text);
	}
	CcPrintfI(output, "{\n");
	output->indent += 4;
	if (sym->semPos) CcSource(output, sym->semPos);
	SCOS_GenCode(self, output, sym->graph, &isChecked);
	output->indent -= 4;
	CcPrintfI(output,"}\n\n");
    }
    CcBitArray_Destruct(&isChecked);
    return TRUE;
}

static CcsBool_t
COS_SynErrors(CcOutputScheme_t * self, CcOutput_t * output)
{
    CcArrayListIter_t iter; char * str;
    const CcSyntaxError_t * synerr;
    const CcArrayList_t * errors = &self->globals->syntax.errors;
    for (synerr = (const CcSyntaxError_t *)CcArrayList_FirstC(errors, &iter);
	 synerr;
	 synerr = (const CcSyntaxError_t *)CcArrayList_NextC(errors, &iter)) {
	CcPrintfI(output, "case %d: s = \"", synerr->base.index);
	str = CcEscape(synerr->sym->name);
	switch (synerr->type) {
	case cet_t:
	    CcPrintf(output, "\\\"\" %s \"\\\" expected", str);
	    break;
	case cet_alt:
	    CcPrintf(output,
		     "this symbol not expected in \\\"\" %s \"\\\"", str);
	    break;
	case cet_sync:
	    CcPrintf(output, "invalid \\\"\" %s \"\\\"", str);
	    break;
	}
	CcFree(str);
	CcPrintf(output, "\"; break;\n");
    }
    return TRUE;
}

static CcsBool_t
COS_InitSet(CcOutputScheme_t * self, CcOutput_t * output)
{
    char * setstr; int setlen, index;
    const CcBitArray_t * cur;
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;

    setlen = self->globals->symtab.terminals.Count;
    setstr = CcMalloc(setlen + 1); setstr[setlen] = 0;
    if (setlen > 4) {
	for (index = 0; index < setlen; ++index)
	    if (index == 0) setstr[index] = '*';
	    else setstr[index] = index % 5 == 0 ? '0' + index % 10 : ' ';
	CcPrintfI(output, "/%s */\n", setstr);
    }
    for (cur = ccself->symSet.start; cur < ccself->symSet.used; ++cur) {
	CcsAssert(setlen == CcBitArray_getCount(cur));
	for (index = 0; index < setlen; ++index)
	    setstr[index] = CcBitArray_Get(cur, index) ? '*' : '.';
	CcPrintfI(output, "\"%s.\"%c /* %d */\n", setstr,
		  cur < ccself->symSet.used - 1 ? ',' : ' ',
		  cur - ccself->symSet.start);
    }
    CcFree(setstr);
    return TRUE;
}

static CcsBool_t
CcCOutputScheme_write(CcOutputScheme_t * self, CcOutput_t * output,
		      const char * func, const char * param)
{
    if (!strcmp(func, "defines")) {
	return COS_Defines(self, output);
    } else if (!strcmp(func, "declarations")) {
	return COS_Declarations(self, output);
    } else if (!strcmp(func, "chars2states")) {
	return COS_Chars2States(self, output);
    } else if (!strcmp(func, "identifiers2keywordkinds")) {
	return COS_Identifiers2KeywordKinds(self, output);
    } else if (!strcmp(func, "comments")) {
	return COS_Comments(self, output);
    } else if (!strcmp(func, "scan1")) {
	return COS_Scan1(self, output);
    } else if (!strcmp(func, "scan3")) {
	return COS_Scan3(self, output);
    } else if (!strcmp(func, "kindUnknownNS")) {
	return COS_KindUnknownNS(self, output);
    } else if (!strcmp(func, "XmlSpecSubList")) {
	return COS_XmlSpecSubList(self, output);
    } else if (!strcmp(func, "XmlSpecList")) {
	return COS_XmlSpecList(self, output);
    } else if (!strcmp(func, "members")) {
	return COS_Members(self, output);
    } else if (!strcmp(func, "constructor")) {
	return COS_Constructor(self, output);
    } else if (!strcmp(func, "destructor")) {
	return COS_Destructor(self, output);
    } else if (!strcmp(func, "Pragmas")) {
	return COS_Pragmas(self, output);
    } else if (!strcmp(func, "ProductionsHeader")) {
	return COS_ProductionsHeader(self, output);
    } else if (!strcmp(func, "ParseRoot")) {
	return COS_ParseRoot(self, output);
    } else if (!strcmp(func, "ProductionsBody")) {
	return COS_ProductionsBody(self, output);
    } else if (!strcmp(func, "SynErrors")) {
	return COS_SynErrors(self, output);
    } else if (!strcmp(func, "InitSet")) {
	return COS_InitSet(self, output);
    }
    fprintf(stderr, "Unknown section '%s' encountered.\n", func);
    return TRUE;
}

static void
CcCOutputScheme_Destruct(CcObject_t * self)
{
    CcCOutputScheme_t * ccself = (CcCOutputScheme_t *)self;
    CcSyntaxSymSet_Destruct(&ccself->symSet);
    CcOutputScheme_Destruct(self);
}

static const CcOutputSchemeType_t COutputSchemeType = {
    { sizeof(CcCOutputScheme_t), "COutputScheme", CcCOutputScheme_Destruct },
    CcCOutputScheme_write
};

CcCOutputScheme_t *
CcCOutputScheme(CcGlobals_t * globals, CcArguments_t * arguments)
{
    CcCOutputScheme_t * self = (CcCOutputScheme_t *)
	CcOutputScheme(&COutputSchemeType, globals, arguments);
    if (globals->base.parser) self->ParserStem = "CcsParser";
    else if (globals->base.xmlparser) self->ParserStem = "CcsXmlParser";
    else { CcsAssert(0); }
    CcSyntaxSymSet(&self->symSet);
    CcSyntaxSymSet_New(&self->symSet, self->base.globals->syntax.allSyncSets);
    return self;
}
