/*---- license ----*/
/*-------------------------------------------------------------------------
Kconfig.atg
Copyright (C) 2008, Charles Wang
Author: Charles Wang  <charlesw123456@gmail.com>
License: LGPLv2
-------------------------------------------------------------------------*/
/*---- enable ----*/
#include  <ctype.h>
#include  "Scanner.h"

/*------------------------------- ScanInput --------------------------------*/
struct KcScanInput_s {
    KcScanInput_t * next;

    KcScanner_t   * scanner;
    char           * fname;
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

#ifdef KcScanner_INDENTATION
    CcsBool_t        lineStart;
    int            * indent;
    int            * indentUsed;
    int            * indentLast;
#endif
};

static CcsToken_t * KcScanInput_NextToken(KcScanInput_t * self);

static CcsBool_t
KcScanInput_Init(KcScanInput_t * self, KcScanner_t * scanner, FILE * fp)
{
    self->scanner = scanner;
    if (!CcsBuffer(&self->buffer, fp)) goto errquit0;
    self->busyTokenList = NULL;
    self->curToken = &self->busyTokenList;
    self->peekToken = &self->busyTokenList;

    self->ch = 0; self->chBytes = 0;
    self->pos = 0; self->line = 1; self->col = 0;
    self->oldEols = 0; self->oldEolsEOL = 0;
#ifdef KcScanner_INDENTATION
    self->lineStart = TRUE;
    if (!(self->indent = CcsMalloc(sizeof(int) * KcScanner_INDENT_START)))
	goto errquit1;
    self->indentUsed = self->indent;
    self->indentLast = self->indent + KcScanner_INDENT_START;
    *self->indentUsed++ = 0;
#endif
    return TRUE;
#ifdef KcScanner_INDENTATION
 errquit1:
    CcsBuffer_Destruct(&self->buffer);
#endif
 errquit0:
    return FALSE;
}

static KcScanInput_t *
KcScanInput(KcScanner_t * scanner, FILE * fp)
{
    KcScanInput_t * self;
    if (!(self = CcsMalloc(sizeof(KcScanInput_t)))) goto errquit0;
    self->next = NULL;
    self->fname = NULL;
    if (!KcScanInput_Init(self, scanner, fp)) goto errquit1;
    return self;
 errquit1:
    CcsFree(self);
 errquit0:
    return NULL;
}

static KcScanInput_t *
KcScanInput_ByName(KcScanner_t * scanner, const char * infn)
{
    FILE * fp;
    KcScanInput_t * self;
    if (!(fp = fopen(infn, "r"))) goto errquit0;
    if (!(self = CcsMalloc(sizeof(KcScanInput_t) + strlen(infn) + 1))) goto errquit1;
    self->next = NULL;
    strcpy(self->fname = (char *)(self + 1), infn);
    if (!KcScanInput_Init(self, scanner, fp)) goto errquit2;
    return self;
 errquit2:
    CcsFree(self);
 errquit1:
    fclose(fp);
 errquit0:
    return NULL;
}

static void
KcScanInput_Destruct(KcScanInput_t * self)
{
    CcsToken_t * cur, * next;

#ifdef KcScanner_INDENTATION
    CcsFree(self->indent);
#endif
    for (cur = self->busyTokenList; cur; cur = next) {
	/* May be trigged by .atg semantic code. */
	CcsAssert(cur->refcnt == 1);
	next = cur->next;
	CcsToken_Destruct(cur);
    }
    CcsBuffer_Destruct(&self->buffer);
    CcsFree(self);
}

static void
KcScanInput_GetCh(KcScanInput_t * self)
{
    if (self->oldEols > 0) {
	self->ch = '\n'; --self->oldEols; self->oldEolsEOL = 1;
    } else {
	if (self->ch == '\n') {
	    if (self->oldEolsEOL) self->oldEolsEOL = 0;
	    else {
		++self->line; self->col = 0;
	    }
#ifdef KcScanner_INDENTATION
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
KcScanInput_Scan(KcScanInput_t * self)
{
    CcsToken_t * cur;
    if (*self->curToken == NULL) {
	*self->curToken = KcScanInput_NextToken(self);
	if (self->curToken == &self->busyTokenList)
	    CcsBuffer_SetBusy(&self->buffer, self->busyTokenList->pos);
    }
    cur = *self->curToken;
    self->peekToken = self->curToken = &cur->next;
    ++cur->refcnt;
    return cur;
}

static CcsToken_t *
KcScanInput_Peek(KcScanInput_t * self)
{
    CcsToken_t * cur;
    do {
	if (*self->peekToken == NULL) {
	    *self->peekToken = KcScanInput_NextToken(self);
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
KcScanInput_ResetPeek(KcScanInput_t * self)
{
    self->peekToken = self->curToken;
}

static void
KcScanInput_IncRef(KcScanInput_t * self, CcsToken_t * token)
{
    ++token->refcnt;
}

static void
KcScanInput_DecRef(KcScanInput_t * self, CcsToken_t * token)
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

static CcsPosition_t *
KcScanInput_GetPosition(KcScanInput_t * self, const CcsToken_t * begin,
			 const CcsToken_t * end)
{
    int len;
    CcsAssert(self == begin->input);
    CcsAssert(self == end->input);
    len = end->pos - begin->pos;
    return CcsPosition(begin->pos, len, begin->col,
		       CcsBuffer_GetString(&self->buffer, begin->pos, len));
}

static CcsPosition_t *
KcScanInput_GetPositionBetween(KcScanInput_t * self,
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
KcScanner_Init(KcScanner_t * self, CcsErrorPool_t * errpool) {
    self->errpool = errpool;
    /*---- declarations ----*/
    self->eofSym = 0;
    self->maxT = 38;
    self->noSym = 38;
    /*---- enable ----*/
    if (!(self->dummyToken =
	  CcsToken(NULL, 0, 0, 0, 0, dummyval, strlen(dummyval))))
	return FALSE;
    return TRUE;
}

KcScanner_t *
KcScanner(KcScanner_t * self, CcsErrorPool_t * errpool, FILE * fp)
{
    if (!(self->cur = KcScanInput(self, fp))) goto errquit0;
    if (!KcScanner_Init(self, errpool)) goto errquit1;
    KcScanInput_GetCh(self->cur);
    return self;
 errquit1:
    KcScanInput_Destruct(self->cur);
 errquit0:
    return NULL;
}

KcScanner_t *
KcScanner_ByName(KcScanner_t * self, CcsErrorPool_t * errpool,
		  const char * fn)
{
    if (!(self->cur = KcScanInput_ByName(self, fn))) goto errquit0;
    if (!KcScanner_Init(self, errpool)) goto errquit1;
    KcScanInput_GetCh(self->cur);
    return self;
 errquit1:
    KcScanInput_Destruct(self->cur);
 errquit0:
    return NULL;
}

void
KcScanner_Destruct(KcScanner_t * self)
{
    KcScanInput_t * cur, * next;
    for (cur = self->cur; cur; cur = next) {
	next = cur->next;
	KcScanInput_Destruct(cur);
    }
    /* May be trigged by .atg semantic code. */
    CcsAssert(self->dummyToken->refcnt == 1);
    CcsToken_Destruct(self->dummyToken);
}

CcsToken_t *
KcScanner_GetDummy(KcScanner_t * self)
{
    KcScanner_IncRef(self, self->dummyToken);
    return self->dummyToken;
}

CcsToken_t *
KcScanner_Scan(KcScanner_t * self)
{
    return KcScanInput_Scan(self->cur);
}

CcsToken_t *
KcScanner_Peek(KcScanner_t * self)
{
    return KcScanInput_Peek(self->cur);
}

void
KcScanner_ResetPeek(KcScanner_t * self)
{
    KcScanInput_ResetPeek(self->cur);
}

void
KcScanner_IncRef(KcScanner_t * self, CcsToken_t * token)
{
    if (token == self->dummyToken) ++token->refcnt;
    else KcScanInput_IncRef(token->input, token);
}

void
KcScanner_DecRef(KcScanner_t * self, CcsToken_t * token)
{
    if (token == self->dummyToken) --token->refcnt;
    else KcScanInput_DecRef(token->input, token);
}

CcsPosition_t *
KcScanner_GetPosition(KcScanner_t * self, const CcsToken_t * begin,
		       const CcsToken_t * end)
{
    return KcScanInput_GetPosition(begin->input, begin, end);
}

CcsPosition_t *
KcScanner_GetPositionBetween(KcScanner_t * self, const CcsToken_t * begin,
			      const CcsToken_t * end)
{
    return KcScanInput_GetPositionBetween(begin->input, begin, end);
}

/*------------------------------- ScanInput --------------------------------*/
/* All the following things are used by KcScanInput_NextToken. */
typedef struct {
    int keyFrom;
    int keyTo;
    int val;
}  Char2State_t;

static const Char2State_t c2sArr[] = {
    /*---- chars2states ----*/
    { EoF, EoF, -1 },
    { 10, 10, 6 },	/* '\n' '\n' */
    { 13, 13, 5 },	/* '\r' '\r' */
    { 33, 33, 25 },	/* '!' '!' */
    { 34, 34, 2 },	/* '"' '"' */
    { 38, 38, 19 },	/* '&' '&' */
    { 40, 40, 21 },	/* '(' '(' */
    { 41, 41, 22 },	/* ')' ')' */
    { 45, 45, 7 },	/* '-' '-' */
    { 61, 61, 23 },	/* '=' '=' */
    { 65, 90, 1 },	/* 'A' 'Z' */
    { 95, 95, 1 },	/* '_' '_' */
    { 97, 122, 1 },	/* 'a' 'z' */
    { 124, 124, 17 },	/* '|' '|' */
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

#ifdef KcScanner_KEYWORD_USED
typedef struct {
    const char * key;
    int val;
}  Identifier2KWKind_t;

static const Identifier2KWKind_t i2kArr[] = {
    /*---- identifiers2keywordkinds ----*/
    { "bool", 17 },
    { "choice", 11 },
    { "comment", 13 },
    { "config", 7 },
    { "def_bool", 23 },
    { "def_tristate", 24 },
    { "default", 22 },
    { "depends", 25 },
    { "endchoice", 12 },
    { "endif", 15 },
    { "endmenu", 10 },
    { "help", 29 },
    { "hex", 19 },
    { "if", 14 },
    { "int", 20 },
    { "mainmenu", 16 },
    { "menu", 9 },
    { "menuconfig", 8 },
    { "on", 26 },
    { "prompt", 21 },
    { "ranges", 28 },
    { "select", 27 },
    { "tristate", 18 },
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
#ifndef KcScanner_CASE_SENSITIVE
    char * cur;
#endif
    char keystr[KcScanner_MAX_KEYWORD_LEN + 1];
    Identifier2KWKind_t * i2k;

    if (keylen > KcScanner_MAX_KEYWORD_LEN) return defaultVal;
    memcpy(keystr, key, keylen);
    keystr[keylen] = 0;
#ifndef KcScanner_CASE_SENSITIVE
    for (cur = keystr; *cur; ++cur) *cur = tolower(*cur);
#endif
    i2k = bsearch(keystr, i2kArr, i2kNum, sizeof(Identifier2KWKind_t), i2kCmp);
    return i2k ? i2k->val : defaultVal;
}

static int
GetKWKind(KcScanInput_t * self, int start, int end, int defaultVal)
{
    return Identifier2KWKind(CcsBuffer_GetString(&self->buffer,
						 start, end - start),
			     end - start, defaultVal);
}
#endif /* KcScanner_KEYWORD_USED */

typedef struct {
    int ch, chBytes;
    int pos, line, col;
}  SLock_t;
static void
KcScanInput_LockCh(KcScanInput_t * self, SLock_t * slock)
{
    slock->ch = self->ch;
    slock->chBytes = self->chBytes;
    slock->pos = self->pos;
    slock->line = self->line;
    slock->col = self->col;
    CcsBuffer_Lock(&self->buffer);
}
static void
KcScanInput_UnlockCh(KcScanInput_t * self, SLock_t * slock)
{
    CcsBuffer_Unlock(&self->buffer);
}
static void
KcScanInput_ResetCh(KcScanInput_t * self, SLock_t * slock)
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
    { { '#', 0 }, { '\n', 0 }, FALSE },
/*---- enable ----*/
};
static const CcsComment_t * commentsLast =
    comments + sizeof(comments) / sizeof(comments[0]);

static CcsBool_t
KcScanInput_Comment(KcScanInput_t * self, const CcsComment_t * c)
{
    SLock_t slock;
    int level = 1, line0 = self->line;

    if (c->start[1]) {
	KcScanInput_LockCh(self, &slock); KcScanInput_GetCh(self);
	if (self->ch != c->start[1]) {
	    KcScanInput_ResetCh(self, &slock);
	    return FALSE;
	}
	KcScanInput_UnlockCh(self, &slock);
    }
    KcScanInput_GetCh(self);
    for (;;) {
	if (self->ch == c->end[0]) {
	    if (c->end[1] == 0) {
		if (--level == 0) break;
	    } else {
		KcScanInput_LockCh(self, &slock); KcScanInput_GetCh(self);
		if (self->ch == c->end[1]) {
		    KcScanInput_UnlockCh(self, &slock);
		    if (--level == 0) break;
		} else {
		    KcScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (c->nested && self->ch == c->start[0]) {
	    if (c->start[1] == 0) {
		++level;
	    } else {
		KcScanInput_LockCh(self, &slock); KcScanInput_GetCh(self);
		if (self->ch == c->start[1]) {
		    KcScanInput_UnlockCh(self, &slock);
		    ++level;
		} else {
		    KcScanInput_ResetCh(self, &slock);
		}
	    }
	} else if (self->ch == EoF) {
	    return TRUE;
	}
	KcScanInput_GetCh(self);
    }
    self->oldEols = self->line - line0;
    KcScanInput_GetCh(self);
    return TRUE;
}

#ifdef KcScanInput_INDENTATION
static CcsToken_t *
KcScanInput_IndentGenerator(KcScanInput_t * self)
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
	    cur = CcsToken(self, KcScanInput_INDENT_OUT, self->pos,
			   self->col, self->line, NULL, 0);
	    cur->next = head; head = cur;
	    --self->indentUsed;
	}
	return head;
    }
    self->lineStart = FALSE;
    if (self->col > self->indentUsed[-1]) {
	if (self->indentUsed == self->indentLast) {
	    newLen = (self->indentLast - self->indent) + KcScanInput_INDENT_START;
	    newIndent = CcsRealloc(self->indent, sizeof(int) * newLen);
	    if (!newIndent) return NULL;
	    self->indentUsed = newIndent + (self->indentUsed - self->indent);
	    self->indentLast = newIndent + newLen;
	    self->indent = newIndent;
	}
	CcsAssert(self->indentUsed < self->indentLast);
	*self->indentUsed++ = self->col;
	return CcsToken(self, KcScanInput_INDENT_IN, self->pos,
			self->col, self->line, NULL, 0);
    }
    for (curIndent = self->indentUsed - 1; self->col < *curIndent; --curIndent);
    if (self->col > *curIndent)
	return CcsToken(self, KcScanInput_INDENT_ERR, self->pos,
			self->col, self->line, NULL, 0);
    head = NULL;
    while (curIndent < self->indentUsed - 1) {
	cur = CcsToken(self, KcScanInput_INDENT_OUT, self->pos,
		       self->col, self->line, NULL, 0);
	cur->next = head; head = cur;
	--self->indentUsed;
    }
    return head;
}
#endif

static CcsToken_t *
KcScanInput_NextToken(KcScanInput_t * self)
{
    int pos, line, col, state, kind; CcsToken_t * t;
    const CcsComment_t * curComment;
    for (;;) {
	while (self->ch == ' '
	       /*---- scan1 ----*/
	       || self->ch == '\t'
	       /*---- enable ----*/
	       ) KcScanInput_GetCh(self);
#ifdef KcScanInput_INDENTATION
	if ((t = KcScanInput_IndentGenerator(self))) return t;
#endif
	for (curComment = comments; curComment < commentsLast; ++curComment)
	    if (self->ch == curComment->start[0] &&
		KcScanInput_Comment(self, curComment)) break;
	if (curComment >= commentsLast) break;
    }
    pos = self->pos; line = self->line; col = self->col;
    CcsBuffer_Lock(&self->buffer);
    state = Char2State(self->ch);
    KcScanInput_GetCh(self);
    switch (state) {
    case -1: kind = self->scanner->eofSym; break;
    case 0: kind = self->scanner->noSym; break;
    /*---- scan3 ----*/
    case 1: case_1:
	if ((self->ch >= '0' && self->ch <= '9') ||
	    (self->ch >= 'A' && self->ch <= 'Z') ||
	    self->ch == '_' ||
	    (self->ch >= 'a' && self->ch <= 'z')) {
	    KcScanInput_GetCh(self); goto case_1;
	} else { kind = GetKWKind(self, pos, self->pos, 134658968); break; }
    case 2: case_2:
	if ((self->ch >= 0 && self->ch <= '\t') ||
	    (self->ch >= '\v' && self->ch <= '\f') ||
	    (self->ch >= 14 && self->ch <= '!') ||
	    (self->ch >= '#' && self->ch <= '[') ||
	    (self->ch >= ']' && self->ch <= 65535)) {
	    KcScanInput_GetCh(self); goto case_2;
	} else if (self->ch == '"') {
	    KcScanInput_GetCh(self); goto case_4;
	} else if (self->ch == '\\') {
	    KcScanInput_GetCh(self); goto case_3;
	} else { kind = self->scanner->noSym; break; }
    case 3: case_3:
	if ((self->ch >= ' ' && self->ch <= '~')) {
	    KcScanInput_GetCh(self); goto case_2;
	} else { kind = self->scanner->noSym; break; }
    case 4: case_4:
	{ kind = 5; break; }
    case 5:
	if (self->ch == '\n') {
	    KcScanInput_GetCh(self); goto case_6;
	} else { kind = self->scanner->noSym; break; }
    case 6: case_6:
	{ kind = 6; break; }
    case 7:
	if (self->ch == '-') {
	    KcScanInput_GetCh(self); goto case_8;
	} else { kind = self->scanner->noSym; break; }
    case 8: case_8:
	if (self->ch == '-') {
	    KcScanInput_GetCh(self); goto case_9;
	} else { kind = self->scanner->noSym; break; }
    case 9: case_9:
	if (self->ch == 'h') {
	    KcScanInput_GetCh(self); goto case_10;
	} else { kind = self->scanner->noSym; break; }
    case 10: case_10:
	if (self->ch == 'e') {
	    KcScanInput_GetCh(self); goto case_11;
	} else { kind = self->scanner->noSym; break; }
    case 11: case_11:
	if (self->ch == 'l') {
	    KcScanInput_GetCh(self); goto case_12;
	} else { kind = self->scanner->noSym; break; }
    case 12: case_12:
	if (self->ch == 'p') {
	    KcScanInput_GetCh(self); goto case_13;
	} else { kind = self->scanner->noSym; break; }
    case 13: case_13:
	if (self->ch == '-') {
	    KcScanInput_GetCh(self); goto case_14;
	} else { kind = self->scanner->noSym; break; }
    case 14: case_14:
	if (self->ch == '-') {
	    KcScanInput_GetCh(self); goto case_15;
	} else { kind = self->scanner->noSym; break; }
    case 15: case_15:
	if (self->ch == '-') {
	    KcScanInput_GetCh(self); goto case_16;
	} else { kind = self->scanner->noSym; break; }
    case 16: case_16:
	{ kind = 30; break; }
    case 17:
	if (self->ch == '|') {
	    KcScanInput_GetCh(self); goto case_18;
	} else { kind = self->scanner->noSym; break; }
    case 18: case_18:
	{ kind = 31; break; }
    case 19:
	if (self->ch == '&') {
	    KcScanInput_GetCh(self); goto case_20;
	} else { kind = self->scanner->noSym; break; }
    case 20: case_20:
	{ kind = 32; break; }
    case 21:
	{ kind = 34; break; }
    case 22:
	{ kind = 35; break; }
    case 23:
	{ kind = 36; break; }
    case 24: case_24:
	{ kind = 37; break; }
    case 25:
	if (self->ch == '=') {
	    KcScanInput_GetCh(self); goto case_24;
	} else { kind = 33; break; }
    /*---- enable ----*/
    }
    t = CcsToken(self, kind, pos, col, line,
		 CcsBuffer_GetString(&self->buffer, pos, self->pos - pos),
		 self->pos - pos);
    CcsBuffer_Unlock(&self->buffer);
    return t;
}