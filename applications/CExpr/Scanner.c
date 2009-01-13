/*---- license ----*/
/*-------------------------------------------------------------------------
c-expr.atg -- atg for c expression input
Copyright (C) 2008, Charles Wang <charlesw123456@gmail.com>
Author: Charles Wang <charlesw123456@gmail.com>

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
-------------------------------------------------------------------------*/
/*---- enable ----*/
#include  <ctype.h>
#include  <limits.h>
#include  "Scanner.h"
#include  "c/IncPathList.h"

/*------------------------------- ScanInput --------------------------------*/
struct CExprScanInput_s {
    CExprScanInput_t * next;

    CExprScanner_t   * scanner;
    char           * fname;
    FILE           * fp;
    CcsBuffer_t      buffer;

    CcsToken_t     * busyTokenList;
    CcsToken_t    ** curToken;
    CcsToken_t    ** peekToken;

    int              ch;
    int              chBytes;
    int              pos;
    int              line;
    int              col;
    int              oldEols;
    int              oldEolsEOL;

#ifdef CExprScanner_INDENTATION
    CcsBool_t        lineStart;
    int            * indent;
    int            * indentUsed;
    int            * indentLast;
    int              indentLimit;
#endif
};

static CcsToken_t * CExprScanInput_NextToken(CExprScanInput_t * self);

static CcsBool_t
CExprScanInput_Init(CExprScanInput_t * self, CExprScanner_t * scanner, FILE * fp)
{
    self->scanner = scanner;
    self->fp = fp;
    if (!CcsBuffer(&self->buffer, fp)) goto errquit0;
    self->busyTokenList = NULL;
    self->curToken = &self->busyTokenList;
    self->peekToken = &self->busyTokenList;

    self->ch = 0; self->chBytes = 0;
    self->pos = 0; self->line = 1; self->col = 0;
    self->oldEols = 0; self->oldEolsEOL = 0;
#ifdef CExprScanner_INDENTATION
    self->lineStart = TRUE;
    if (!(self->indent = CcsMalloc(sizeof(int) * CExprScanner_INDENT_START)))
	goto errquit1;
    self->indentUsed = self->indent;
    self->indentLast = self->indent + CExprScanner_INDENT_START;
    *self->indentUsed++ = 0;
    self->indentLimit = -1;
#endif
    return TRUE;
#ifdef CExprScanner_INDENTATION
 errquit1:
    CcsBuffer_Destruct(&self->buffer);
#endif
 errquit0:
    return FALSE;
}

static CExprScanInput_t *
CExprScanInput(CExprScanner_t * scanner, FILE * fp)
{
    CExprScanInput_t * self;
    if (!(self = CcsMalloc(sizeof(CExprScanInput_t)))) goto errquit0;
    self->next = NULL;
    self->fname = NULL;
    if (!CExprScanInput_Init(self, scanner, fp)) goto errquit1;
    return self;
 errquit1:
    CcsFree(self);
 errquit0:
    return NULL;
}

static CExprScanInput_t *
CExprScanInput_ByName(CExprScanner_t * scanner, const CcsIncPathList_t * list,
		    const char * includer, const char * infn)
{
    FILE * fp;
    CExprScanInput_t * self;
    char infnpath[PATH_MAX];
    if (!(fp = CcsIncPathList_Open(list, infnpath, sizeof(infnpath),
				   includer, infn)))
	goto errquit0;
    if (!(self = CcsMalloc(sizeof(CExprScanInput_t) + strlen(infnpath) + 1)))
	goto errquit1;
    self->next = NULL;
    strcpy(self->fname = (char *)(self + 1), infnpath);
    if (!CExprScanInput_Init(self, scanner, fp)) goto errquit2;
    return self;
 errquit2:
    CcsFree(self);
 errquit1:
    fclose(fp);
 errquit0:
    return NULL;
}

static void
CExprScanInput_Destruct(CExprScanInput_t * self)
{
    CcsToken_t * cur, * next;

#ifdef CExprScanner_INDENTATION
    CcsFree(self->indent);
#endif
    for (cur = self->busyTokenList; cur; cur = next) {
	/* May be trigged by .atg semantic code. */
	CcsAssert(cur->refcnt == 1);
	next = cur->next;
	CcsToken_Destruct(cur);
    }
    CcsBuffer_Destruct(&self->buffer);
    if (self->fname) fclose(self->fp);
    CcsFree(self);
}

static void
CExprScanInput_GetCh(CExprScanInput_t * self)
{
    if (self->oldEols > 0) {
	self->ch = '\n'; --self->oldEols; self->oldEolsEOL = 1;
    } else {
	if (self->ch == '\n') {
	    if (self->oldEolsEOL) self->oldEolsEOL = 0;
	    else {
		++self->line; self->col = 0;
	    }
#ifdef CExprScanner_INDENTATION
	    self->lineStart = TRUE;
#endif
	} else if (self->ch == '\t') {
	    self->col += 8 - self->col % 8;
	} else {
	    /* FIX ME: May be the width of some specical character
	     * is NOT self->chBytes. */
	    self->col += self->chBytes;
	}
	self->ch = CcsBuffer_Read(&self->buffer, &self->chBytes);
	self->pos = CcsBuffer_GetPos(&self->buffer);
    }
}

static CcsToken_t *
CExprScanInput_Scan(CExprScanInput_t * self)
{
    CcsToken_t * cur;
    if (*self->curToken == NULL) {
	*self->curToken = CExprScanInput_NextToken(self);
	if (self->curToken == &self->busyTokenList)
	    CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
    }
    cur = *self->curToken;
    self->peekToken = self->curToken = &cur->next;
    ++cur->refcnt;
    return cur;
}

static CcsToken_t *
CExprScanInput_Peek(CExprScanInput_t * self)
{
    CcsToken_t * cur;
    do {
	if (*self->peekToken == NULL) {
	    *self->peekToken = CExprScanInput_NextToken(self);
	    if (self->peekToken == &self->busyTokenList)
		CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
	}
	cur = *self->peekToken;
	self->peekToken = &cur->next;
    } while (cur->kind > self->scanner->maxT); /* Skip pragmas */
    ++cur->refcnt;
    return cur;
}

static void
CExprScanInput_ResetPeek(CExprScanInput_t * self)
{
    self->peekToken = self->curToken;
}

static void
CExprScanInput_IncRef(CExprScanInput_t * self, CcsToken_t * token)
{
    ++token->refcnt;
}

static void
CExprScanInput_DecRef(CExprScanInput_t * self, CcsToken_t * token)
{
    if (--token->refcnt > 1) return;
    CcsAssert(token->refcnt == 1);
    if (token != self->busyTokenList) return;
    /* Detach all tokens which is refered by self->busyTokenList only. */
    while (token && token->refcnt <= 1) {
	CcsAssert(token->refcnt == 1);
	/* Detach token. */
	if (self->curToken == &token->next)
	    self->curToken = &self->busyTokenList;
	if (self->peekToken == &token->next)
	    self->peekToken = &self->busyTokenList;
	self->busyTokenList = token->next;
	CcsToken_Destruct(token);
	token = self->busyTokenList;
    }
    /* Adjust CcsBuffer busy pointer */
    if (self->busyTokenList) {
	CcsAssert(self->busyTokenList->refcnt > 1);
	CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
    } else {
	CcsBuffer_ClearBusy(&self->buffer);
    }
}

#ifdef CExprScanner_INDENTATION
static void
CExprScanInput_IndentLimit(CExprScanInput_t * self, const CcsToken_t * indentIn)
{
    CcsAssert(indentIn->kind == CExprScanner_INDENT_IN);
    self->indentLimit = indentIn->loc.col;
}
#endif

static CcsPosition_t *
CExprScanInput_GetPosition(CExprScanInput_t * self, const CcsToken_t * begin,
			 const CcsToken_t * end)
{
    int len;
    CcsAssert(self == begin->input);
    CcsAssert(self == end->input);
    len = end->pos - begin->pos;
    return CcsPosition(begin->pos, len, begin->loc.col,
		       CcsBuffer_GetString(&self->buffer, begin->pos, len));
}

static CcsPosition_t *
CExprScanInput_GetPositionBetween(CExprScanInput_t * self,
				const CcsToken_t * begin,
				const CcsToken_t * end)
{
    int begpos, len;
    CcsAssert(self == begin->input);
    CcsAssert(self == end->input);
    begpos = begin->pos + strlen(begin->val);
    len = end->pos - begpos;
    const char * start = CcsBuffer_GetString(&self->buffer, begpos, len);
    const char * cur, * last = start + len;

    /* Skip the leading spaces. */
    for (cur = start; cur < last; ++cur)
	if (*cur != ' ' && *cur != '\t' && *cur != '\r' && *cur != '\n') break;
    return CcsPosition(begpos + (cur - start), last - cur, 0, cur);
}

/*------------------------------- Scanner --------------------------------*/
static const char * dummyval = "dummy";

static CcsBool_t
CExprScanner_Init(CExprScanner_t * self, CcsErrorPool_t * errpool) {
    self->errpool = errpool;
    /*---- declarations ----*/
    self->eofSym = 0;
    self->maxT = 24;
    self->noSym = 24;
    /*---- enable ----*/
    if (!(self->dummyToken =
	  CcsToken(NULL, 0, NULL, 0, 0, 0, dummyval, strlen(dummyval))))
	return FALSE;
    return TRUE;
}

CExprScanner_t *
CExprScanner(CExprScanner_t * self, CcsErrorPool_t * errpool, FILE * fp)
{
    if (!(self->cur = CExprScanInput(self, fp))) goto errquit0;
    if (!CExprScanner_Init(self, errpool)) goto errquit1;
    CExprScanInput_GetCh(self->cur);
    return self;
 errquit1:
    CExprScanInput_Destruct(self->cur);
 errquit0:
    return NULL;
}

CExprScanner_t *
CExprScanner_ByName(CExprScanner_t * self, CcsErrorPool_t * errpool,
		  const char * fn)
{
    if (!(self->cur = CExprScanInput_ByName(self, NULL, NULL, fn)))
	goto errquit0;
    if (!CExprScanner_Init(self, errpool)) goto errquit1;
    CExprScanInput_GetCh(self->cur);
    return self;
 errquit1:
    CExprScanInput_Destruct(self->cur);
 errquit0:
    return NULL;
}

void
CExprScanner_Destruct(CExprScanner_t * self)
{
    CExprScanInput_t * cur, * next;
    for (cur = self->cur; cur; cur = next) {
	next = cur->next;
	CExprScanInput_Destruct(cur);
    }
    /* May be trigged by .atg semantic code. */
    CcsAssert(self->dummyToken->refcnt == 1);
    CcsToken_Destruct(self->dummyToken);
}

CcsToken_t *
CExprScanner_GetDummy(CExprScanner_t * self)
{
    CExprScanner_IncRef(self, self->dummyToken);
    return self->dummyToken;
}

CcsToken_t *
CExprScanner_Scan(CExprScanner_t * self)
{
    CcsToken_t * token; CExprScanInput_t * next;
    for (;;) {
	token = CExprScanInput_Scan(self->cur);
	if (token->kind != self->eofSym) break;
	if (self->cur->next == NULL) break;
	CExprScanInput_DecRef(token->input, token);
	next = self->cur->next;
	CExprScanInput_Destruct(self->cur);
	self->cur = next;
    }
    return token;
}

CcsToken_t *
CExprScanner_Peek(CExprScanner_t * self)
{
    CcsToken_t * token; CExprScanInput_t * cur;
    cur = self->cur;
    for (;;) {
	token = CExprScanInput_Peek(self->cur);
	if (token->kind != self->eofSym) break;
	if (cur->next == NULL) break;
	CExprScanInput_DecRef(token->input, token);
	cur = cur->next;
    }
    return token;
}

void
CExprScanner_ResetPeek(CExprScanner_t * self)
{
    CExprScanInput_t * cur;
    for (cur = self->cur; cur; cur = cur->next)
	CExprScanInput_ResetPeek(cur);
}

void
CExprScanner_IncRef(CExprScanner_t * self, CcsToken_t * token)
{
    if (token == self->dummyToken) ++token->refcnt;
    else CExprScanInput_IncRef(token->input, token);
}

void
CExprScanner_DecRef(CExprScanner_t * self, CcsToken_t * token)
{
    if (token == self->dummyToken) --token->refcnt;
    else CExprScanInput_DecRef(token->input, token);
}

#ifdef CExprScanner_INDENTATION
void
CExprScanner_IndentLimit(CExprScanner_t * self, const CcsToken_t * indentIn)
{
    CcsAssert(indentIn->input == self->cur);
    CExprScanInput_IndentLimit(self->cur, indentIn);
}
#endif

CcsPosition_t *
CExprScanner_GetPosition(CExprScanner_t * self, const CcsToken_t * begin,
		       const CcsToken_t * end)
{
    return CExprScanInput_GetPosition(begin->input, begin, end);
}

CcsPosition_t *
CExprScanner_GetPositionBetween(CExprScanner_t * self, const CcsToken_t * begin,
			      const CcsToken_t * end)
{
    return CExprScanInput_GetPositionBetween(begin->input, begin, end);
}

CcsBool_t
CExprScanner_Include(CExprScanner_t * self, FILE * fp)
{
    CExprScanInput_t * input;
    if (!(input = CExprScanInput(self, fp))) return FALSE;
    input->next = self->cur;
    self->cur = input;
    return TRUE;
}

CcsBool_t
CExprScanner_IncludeByName(CExprScanner_t * self, const CcsIncPathList_t * list,
			 const char * infn)
{
    CExprScanInput_t * input;
    if (!(input = CExprScanInput_ByName(self, list, self->cur->fname, infn)))
	return FALSE;
    input->next = self->cur;
    self->cur = input;
    return TRUE;
}

/*------------------------------- ScanInput --------------------------------*/
/* All the following things are used by CExprScanInput_NextToken. */
typedef struct {
    int keyFrom;
    int keyTo;
    int val;
}  Char2State_t;

static const Char2State_t c2sArr[] = {
    /*---- chars2states ----*/
    { EoF, EoF, -1 },
    { 33, 33, 9 },	/* '!' '!' */
    { 37, 37, 19 },	/* '%' '%' */
    { 38, 38, 23 },	/* '&' '&' */
    { 40, 40, 20 },	/* '(' '(' */
    { 41, 41, 21 },	/* ')' ')' */
    { 42, 42, 17 },	/* '*' '*' */
    { 43, 43, 15 },	/* '+' '+' */
    { 45, 45, 16 },	/* '-' '-' */
    { 47, 47, 18 },	/* '/' '/' */
    { 48, 57, 1 },	/* '0' '9' */
    { 58, 58, 3 },	/* ':' ':' */
    { 60, 60, 25 },	/* '<' '<' */
    { 61, 61, 7 },	/* '=' '=' */
    { 62, 62, 24 },	/* '>' '>' */
    { 63, 63, 2 },	/* '?' '?' */
    { 94, 94, 6 },	/* '^' '^' */
    { 124, 124, 22 },	/* '|' '|' */
    /*---- enable ----*/
};
static const int c2sNum = sizeof(c2sArr) / sizeof(c2sArr[0]);

static int
c2sCmp(const void * key, const void * c2s)
{
    int keyval = *(const int *)key;
    const Char2State_t * ccc2s = (const Char2State_t *)c2s;
    if (keyval < ccc2s->keyFrom) return -1;
    if (keyval > ccc2s->keyTo) return 1;
    return 0;
}
static int
Char2State(int chr)
{
    Char2State_t * c2s;

    c2s = bsearch(&chr, c2sArr, c2sNum, sizeof(Char2State_t), c2sCmp);
    return c2s ? c2s->val : 0;
}

#ifdef CExprScanner_KEYWORD_USED
typedef struct {
    const char * key;
    int val;
}  Identifier2KWKind_t;

static const Identifier2KWKind_t i2kArr[] = {
    /*---- identifiers2keywordkinds ----*/
    /*---- enable ----*/
};
static const int i2kNum = sizeof(i2kArr) / sizeof(i2kArr[0]);

static int
i2kCmp(const void * key, const void * i2k)
{
    return strcmp((const char *)key, ((const Identifier2KWKind_t *)i2k)->key);
}

static int
Identifier2KWKind(const char * key, size_t keylen, int defaultVal)
{
#ifndef CExprScanner_CASE_SENSITIVE
    char * cur;
#endif
    char keystr[CExprScanner_MAX_KEYWORD_LEN + 1];
    Identifier2KWKind_t * i2k;

    if (keylen > CExprScanner_MAX_KEYWORD_LEN) return defaultVal;
    memcpy(keystr, key, keylen);
    keystr[keylen] = 0;
#ifndef CExprScanner_CASE_SENSITIVE
    for (cur = keystr; *cur; ++cur) *cur = tolower(*cur);
#endif
    i2k = bsearch(keystr, i2kArr, i2kNum, sizeof(Identifier2KWKind_t), i2kCmp);
    return i2k ? i2k->val : defaultVal;
}

static int
GetKWKind(CExprScanInput_t * self, int start, int end, int defaultVal)
{
    return Identifier2KWKind(CcsBuffer_GetString(&self->buffer,
						 start, end - start),
			     end - start, defaultVal);
}
#endif /* CExprScanner_KEYWORD_USED */

typedef struct {
    int ch, chBytes;
    int pos, line, col;
}  SLock_t;
static void
CExprScanInput_LockCh(CExprScanInput_t * self, SLock_t * slock)
{
    slock->ch = self->ch;
    slock->chBytes = self->chBytes;
    slock->pos = self->pos;
    slock->line = self->line;
    slock->col = self->col;
    CcsBuffer_Lock(&self->buffer);
}
static void
CExprScanInput_UnlockCh(CExprScanInput_t * self, SLock_t * slock)
{
    CcsBuffer_Unlock(&self->buffer);
}
static void
CExprScanInput_ResetCh(CExprScanInput_t * self, SLock_t * slock)
{
    self->ch = slock->ch;
    self->chBytes = slock->chBytes;
    self->pos = slock->pos;
    self->line = slock->line;
    CcsBuffer_LockReset(&self->buffer);
}

typedef struct {
    int start[2];
    int end[2];
    CcsBool_t nested;
}  CcsComment_t;

static const CcsComment_t comments[] = {
/*---- comments ----*/
/*---- enable ----*/
};
static const CcsComment_t * commentsLast =
    comments + sizeof(comments) / sizeof(comments[0]);

static CcsBool_t
CExprScanInput_Comment(CExprScanInput_t * self, const CcsComment_t * c)
{
    SLock_t slock;
    int level = 1, line0 = self->line;

    if (c->start[1]) {
	CExprScanInput_LockCh(self, &slock); CExprScanInput_GetCh(self);
	if (self->ch != c->start[1]) {
	    CExprScanInput_ResetCh(self, &slock);
	    return FALSE;
	}
	CExprScanInput_UnlockCh(self, &slock);
    }
    CExprScanInput_GetCh(self);
    for (;;) {
	if (self->ch == c->end[0]) {
	    if (c->end[1] == 0) {
		if (--level == 0) break;
	    } else {
		CExprScanInput_LockCh(self, &slock); CExprScanInput_GetCh(self);
		if (self->ch == c->end[1]) {
		    CExprScanInput_UnlockCh(self, &slock);
		    if (--level == 0) break;
		} else {
		    CExprScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (c->nested && self->ch == c->start[0]) {
	    if (c->start[1] == 0) {
		++level;
	    } else {
		CExprScanInput_LockCh(self, &slock); CExprScanInput_GetCh(self);
		if (self->ch == c->start[1]) {
		    CExprScanInput_UnlockCh(self, &slock);
		    ++level;
		} else {
		    CExprScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (self->ch == EoF) {
	    return TRUE;
	}
	CExprScanInput_GetCh(self);
    }
    self->oldEols = self->line - line0;
    CExprScanInput_GetCh(self);
    return TRUE;
}

#ifdef CExprScanner_INDENTATION
static CcsToken_t *
CExprScanInput_IndentGenerator(CExprScanInput_t * self)
{
    int newLen; int * newIndent, * curIndent;
    CcsToken_t * head, * cur;

    if (!self->lineStart) return NULL;
    CcsAssert(self->indent < self->indentUsed);
    /* Skip blank lines. */
    if (self->ch == '\r' || self->ch == '\n') return NULL;
    /* Dump all required IndentOut when EoF encountered. */
    if (self->ch == EoF) {
	head = NULL;
	while (self->indent < self->indentUsed - 1) {
	    cur = CcsToken(self, CExprScanner_INDENT_OUT, self->fname, self->pos,
			   self->line, self->col, NULL, 0);
	    cur->next = head; head = cur;
	    --self->indentUsed;
	}
	return head;
    }
    if (self->indentLimit != -1 && self->col >= self->indentLimit) return NULL;
    self->indentLimit = -1;
    self->lineStart = FALSE;
    if (self->col > self->indentUsed[-1]) {
	if (self->indentUsed == self->indentLast) {
	    newLen = (self->indentLast - self->indent) + CExprScanner_INDENT_START;
	    newIndent = CcsRealloc(self->indent, sizeof(int) * newLen);
	    if (!newIndent) return NULL;
	    self->indentUsed = newIndent + (self->indentUsed - self->indent);
	    self->indentLast = newIndent + newLen;
	    self->indent = newIndent;
	}
	CcsAssert(self->indentUsed < self->indentLast);
	*self->indentUsed++ = self->col;
	return CcsToken(self, CExprScanner_INDENT_IN, self->fname, self->pos,
			self->line, self->col, NULL, 0);
    }
    for (curIndent = self->indentUsed - 1; self->col < *curIndent; --curIndent);
    if (self->col > *curIndent)
	return CcsToken(self, CExprScanner_INDENT_ERR, self->fname, self->pos,
			self->line, self->col, NULL, 0);
    head = NULL;
    while (curIndent < self->indentUsed - 1) {
	cur = CcsToken(self, CExprScanner_INDENT_OUT, self->fname, self->pos,
		       self->line, self->col, NULL, 0);
	cur->next = head; head = cur;
	--self->indentUsed;
    }
    return head;
}
#endif

static CcsToken_t *
CExprScanInput_NextToken(CExprScanInput_t * self)
{
    int pos, line, col, state, kind; CcsToken_t * t;
    const CcsComment_t * curComment;
    for (;;) {
	while (self->ch == ' '
	       /*---- scan1 ----*/
	       || (self->ch >= '\t' && self->ch <= '\n')
	       || self->ch == '\r'
	       /*---- enable ----*/
	       ) CExprScanInput_GetCh(self);
#ifdef CExprScanner_INDENTATION
	if ((t = CExprScanInput_IndentGenerator(self))) return t;
#endif
	for (curComment = comments; curComment < commentsLast; ++curComment)
	    if (self->ch == curComment->start[0] &&
		CExprScanInput_Comment(self, curComment)) break;
	if (curComment >= commentsLast) break;
    }
    pos = self->pos; line = self->line; col = self->col;
    CcsBuffer_Lock(&self->buffer);
    state = Char2State(self->ch);
    CExprScanInput_GetCh(self);
    kind = -2; /* Avoid gcc warning */
    switch (state) {
    case -1: kind = self->scanner->eofSym; break;
    case 0: kind = self->scanner->noSym; break;
    /*---- scan3 ----*/
    case 1: case_1:
	if ((self->ch >= '0' && self->ch <= '9')) {
	    CExprScanInput_GetCh(self); goto case_1;
	} else { kind = 1; break; }
    case 2:
	{ kind = 2; break; }
    case 3:
	{ kind = 3; break; }
    case 4: case_4:
	{ kind = 4; break; }
    case 5: case_5:
	{ kind = 5; break; }
    case 6:
	{ kind = 7; break; }
    case 7:
	if (self->ch == '=') {
	    CExprScanInput_GetCh(self); goto case_8;
	} else { kind = self->scanner->noSym; break; }
    case 8: case_8:
	{ kind = 9; break; }
    case 9:
	if (self->ch == '=') {
	    CExprScanInput_GetCh(self); goto case_10;
	} else { kind = self->scanner->noSym; break; }
    case 10: case_10:
	{ kind = 10; break; }
    case 11: case_11:
	{ kind = 12; break; }
    case 12: case_12:
	{ kind = 14; break; }
    case 13: case_13:
	{ kind = 15; break; }
    case 14: case_14:
	{ kind = 16; break; }
    case 15:
	{ kind = 17; break; }
    case 16:
	{ kind = 18; break; }
    case 17:
	{ kind = 19; break; }
    case 18:
	{ kind = 20; break; }
    case 19:
	{ kind = 21; break; }
    case 20:
	{ kind = 22; break; }
    case 21:
	{ kind = 23; break; }
    case 22:
	if (self->ch == '|') {
	    CExprScanInput_GetCh(self); goto case_4;
	} else { kind = 6; break; }
    case 23:
	if (self->ch == '&') {
	    CExprScanInput_GetCh(self); goto case_5;
	} else { kind = 8; break; }
    case 24:
	if (self->ch == '=') {
	    CExprScanInput_GetCh(self); goto case_11;
	} else if (self->ch == '>') {
	    CExprScanInput_GetCh(self); goto case_14;
	} else { kind = 11; break; }
    case 25:
	if (self->ch == '=') {
	    CExprScanInput_GetCh(self); goto case_12;
	} else if (self->ch == '<') {
	    CExprScanInput_GetCh(self); goto case_13;
	} else { kind = 13; break; }
    /*---- enable ----*/
    }
    CcsAssert(kind != -2);
    t = CcsToken(self, kind, self->fname, pos, line, col,
		 CcsBuffer_GetString(&self->buffer, pos, self->pos - pos),
		 self->pos - pos);
    CcsBuffer_Unlock(&self->buffer);
    return t;
}
