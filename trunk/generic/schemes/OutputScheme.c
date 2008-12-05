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
#include  <ctype.h>
#include  <errno.h>
#include  <libgen.h>
#include  <limits.h>
#include  <unistd.h>
#include  "OutputScheme.h"
#include  "Arguments.h"
#include  "String.h"

static const char * _8tab_ = "\t\t\t\t\t\t\t\t";
static const char * _7space_ = "       ";

static void
WriteSpaces(FILE * outfp, int spaces)
{
    while (spaces >= 64) {
	fprintf(outfp, "%s", _8tab_);
	spaces -= 64;
    }
    if (spaces >= 8) {
	fprintf(outfp, "%s", _8tab_ + (8 - spaces / 8));
	spaces %= 8;
    }
    if (spaces > 0)
	fprintf(outfp, "%s", _7space_ + (7 - spaces));
}

void
CcPrintf(CcOutput_t * self, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(self->outfp, format, ap); 
    va_end(ap);
}
void
CcPrintfI(CcOutput_t * self, const char * format, ...)
{
    va_list ap;
    WriteSpaces(self->outfp, self->indent);
    va_start(ap, format);
    vfprintf(self->outfp, format, ap); 
    va_end(ap);
}

static const char *
GetEOL(const char * start, char * eol)
{
    const char * end = start + strcspn(start, "\r\n");
    if (*end == 0) return end;
    *eol++ = end[0]; *eol = 0;
    if (end[0] == end[1] || (end[1] != '\r' && end[1] != '\n')) return end + 1;
    *eol++ = end[1]; *eol = 0;
    return end + 2;
}

void
CcSource(CcOutput_t * self, const CcsPosition_t * pos)
{
    char eol[3]; int curcol;
    const char * start, * cur, * end;
    CcsBool_t isBlank, hasEOL;

    eol[0] = 0;
    start = pos->text; end = GetEOL(start, eol);
    WriteSpaces(self->outfp, self->indent);
    fwrite(start, end - start, 1, self->outfp);
    isBlank = FALSE;
    hasEOL = pos->text < end && (end[-1] == '\r' || end[-1] == '\n');

    while (*end) {
	start = end; end = GetEOL(start, eol);
	curcol = 0;
	for (cur = start; *cur == ' ' || *cur == '\t'; ++cur)
	    curcol += (*cur == ' ' ? 1 : 8 - curcol % 8);
	isBlank = *cur == 0 || *cur == '\r' || *cur == '\n';
	if (!isBlank) {
	    /* Only non-blank line is outputed */
	    if (curcol <= pos->col) WriteSpaces(self->outfp, self->indent);
	    else WriteSpaces(self->outfp, self->indent + (curcol - pos->col));
	    fwrite(cur, end - cur, 1, self->outfp);
	    hasEOL = start < end && (end[-1] == '\r' || end[-1] == '\n');
	}
    }
    if (!isBlank && !hasEOL) {
	/* There is no EOL in the last line, add it */
	if (eol[0]) fwrite(eol, strlen(eol), 1, self->outfp);
	else fputc('\n', self->outfp);
    }
}

CcOutputScheme_t *
CcOutputScheme(const CcOutputSchemeType_t * type, CcGlobals_t * globals,
	       CcArguments_t * arguments)
{
    CcOutputScheme_t * self = (CcOutputScheme_t *)CcObject(&type->base);
    self->globals = globals;
    self->arguments = arguments;
    return (CcOutputScheme_t *)self;
}

void
CcOutputScheme_Destruct(CcObject_t * self)
{
    CcObject_Destruct(self);
}

static void
MakePath(char * path, size_t szpath, const char * dir, const char * bn)
{
    if (dir)
	snprintf(path, szpath, "%s/%s", dir, bn);
    else
	snprintf(path, szpath, "%s", bn);
}

typedef struct {
    const char * extension;
    const char * start;
    const char * end;
}  CommentMark_t;

static const CommentMark_t cmarr[] = {
    { ".html", "<!---- ", " ---->" },
    { NULL, "/*---- ", " ----*/" }
};

static const CommentMark_t *
Path2CommentMark(const char * tempPath)
{
    int l0 = strlen(tempPath), l1; const CommentMark_t * curcm;
    for (curcm = cmarr; curcm->extension; ++curcm) {
	l1 = strlen(curcm->extension);
	if (l0 > l1 && !strcmp(tempPath + (l0 - l1), curcm->extension))
	    return curcm;
    }
    return curcm;
}

static CcsBool_t
LocateMark(const char ** b, const char ** e,
	   const char * lmark, const char * rmark)
{
    int llen = strlen(lmark), rlen = strlen(rmark);
    int tlen = llen + rlen;
    while (*b < *e && isspace(**b)) ++*b;
    while (*b < *e && isspace(*(*e - 1))) --*e;
    if (*e - *b < tlen) return FALSE;
    if (strncmp(*b, lmark, llen)) return FALSE;
    if (strncmp(*e - rlen, rmark, rlen)) return FALSE;
    *b += llen; *e -= rlen;
    while (*b < *e && isspace(**b)) ++*b;
    while (*b < *e && isspace(*(*e - 1))) --*e;
    return TRUE;
}

static void
GetResult(char * dest, size_t destlen, const char * src, size_t srclen)
{
    if (srclen < destlen - 1) {
	memcpy(dest, src, srclen); dest[srclen] = 0;
    } else {
	memcpy(dest, src, destlen - 1); dest[destlen - 1] = 0;
    }
}

static CcsBool_t
CheckMark(const char * lnbuf, const char * startMark, const char * endMark,
	  int * retIndent, char * retCommand, size_t szRetCommand,
	  char * retParamStr, size_t szRetParamStr)
{
    const char * b, * e, * start;
    if (!*lnbuf) return FALSE;
    b = lnbuf; e = lnbuf + strlen(lnbuf);

    *retIndent = 0;
    for (b = lnbuf; *b == ' ' || *b == '\t'; ++b)
	*retIndent += (*b == ' ' ? 1 : 8);

    if (!LocateMark(&b, &e, startMark, endMark)) return FALSE;
    start = b;
    while (b < e && isalnum(*b)) ++b;
    GetResult(retCommand, szRetCommand, start, b - start);

    while (b < e && isspace(*b)) ++b;
    if (*b != '(') *retParamStr = 0;
    else {
	start = b + 1;
	while (b <= e && *b != ')') ++b;
	GetResult(retParamStr, szRetParamStr, start, b - start);
	++b;
    }
    return TRUE;
}

static void
TextWritter(FILE * outfp, char * lnbuf,
	    const char * replacedPrefix, const char * prefix)
{
    CcsBool_t WordStart; char * start, * cur; int len;
    if (prefix == NULL || !*replacedPrefix ||
	!strcmp(replacedPrefix, prefix)) {
	fputs(lnbuf, outfp); return;
    }
    len = strlen(replacedPrefix);
    start = cur = lnbuf; WordStart = TRUE;
    while (*cur) {
	if (!WordStart ||
	    *cur != *replacedPrefix ||
	    strncmp(cur, replacedPrefix, len)) {
	    WordStart = !isalnum(*cur) && *cur != '_';
	    ++cur;
	} else { /* An instance of replacedPrefix is found. */
	    *cur = 0;
	    fprintf(outfp, "%s%s", start, prefix);
	    start = (cur += len);
	    WordStart = FALSE;
	}
    }
    fputs(start, outfp);
}

static CcsBool_t
CcOutputScheme_ApplyTemplate(CcOutputScheme_t * self, const char * tempPath,
			     const char * outPath, const char * prefix)
{
    char tempOutPath[PATH_MAX];
    FILE * tempfp, * outfp;
    char lnbuf[4096]; CcsBool_t enabled;
    char Command[128], ParamStr[128], replacedPrefix[128];
    CcOutput_t output;
    const CcsPosition_t * pos;
    const CommentMark_t * tempCM = Path2CommentMark(tempPath);
    const CcOutputSchemeType_t * type =
	(const CcOutputSchemeType_t *)self->base.type;

    if (!(tempfp = fopen(tempPath, "r"))) {
	fprintf(stderr, "open %s for read failed.\n", tempPath);
	goto errquit0;
    }
    if (!strcmp(tempPath, outPath)) {
	snprintf(tempOutPath, sizeof(tempOutPath), "%s.out", outPath);
	if (!(outfp = fopen(tempOutPath, "w"))) {
	    fprintf(stderr, "open %s for write failed.\n", tempOutPath);
	    goto errquit1;
	}
    } else {
	tempOutPath[0] = 0;
	if (!(outfp = fopen(outPath, "w"))) {
	    fprintf(stderr, "open %s for write failed.\n", outPath);
	    goto errquit1;
	}
    }

    printf("Updating %s to %s.....\n", tempPath, outPath);
    output.outfp = outfp;
    enabled = TRUE; replacedPrefix[0] = 0;
    while (fgets(lnbuf, sizeof(lnbuf), tempfp)) {
	if (!CheckMark(lnbuf, tempCM->start, tempCM->end, &output.indent,
		       Command, sizeof(Command), ParamStr, sizeof(ParamStr))) {
	    /* Common line */
	    if (enabled) TextWritter(outfp, lnbuf, replacedPrefix, prefix);
	    continue;
	}
	fputs(lnbuf, outfp);
	if (!strcmp(Command, "prefix")) {
	    snprintf(replacedPrefix, sizeof(replacedPrefix), "%s", ParamStr);
	} else if (!strcmp(Command, "enable")) {
	    enabled = TRUE;
	} else if ((pos = CcGlobals_GetSection(self->globals, Command))) {
	    CcPrintf(&output, "%s", pos->text);
	    enabled = FALSE;
	} else {
	    if (!type->write(self, &output, Command, ParamStr))
		goto errquit2;
	    enabled = FALSE;
	}
    }

    fclose(outfp);
    fclose(tempfp);

    if (tempOutPath[0]) {
	if (rename(tempOutPath, outPath) < 0) {
	    fprintf(stderr, "Rename failed: %s\n", strerror(errno));
	    return FALSE;
	}
    }
    return TRUE;
 errquit2:
    fclose(outfp);
    if (tempOutPath[0]) remove(tempOutPath);
 errquit1:
    fclose(tempfp);
 errquit0:
    return FALSE;
}

CcsBool_t
CcOutputScheme_GenerateOutputs(CcOutputScheme_t * self)
{
    const char * prefix, * updatePath; CcArgumentsIter_t iter;
    const char * updateDir; CcArrayListIter_t iter0;
    const CcArrayList_t * updates = &self->globals->updates;
    const CcString_t * updateFile; char updatePath0[PATH_MAX];

    prefix = CcArguments_First(self->arguments, "prefix", &iter);
    for (updatePath = CcArguments_First(self->arguments, "u", &iter);
	 updatePath; updatePath = CcArguments_Next(self->arguments, &iter))
	if (!CcOutputScheme_ApplyTemplate(self, updatePath, updatePath, prefix)) 
	    return FALSE;
    for (updateDir = CcArguments_First(self->arguments, "ud", &iter);
	 updateDir; updateDir = CcArguments_Next(self->arguments, &iter))
	for (updateFile = (const CcString_t *)CcArrayList_FirstC(updates, &iter0);
	     updateFile; updateFile = (const CcString_t *)CcArrayList_NextC(updates, &iter0)) {
	    MakePath(updatePath0, sizeof(updatePath0), updateDir, updateFile->value);
	    if (!CcOutputScheme_ApplyTemplate(self, updatePath0, updatePath0, prefix))
		return FALSE;
	}
    return TRUE;
}
