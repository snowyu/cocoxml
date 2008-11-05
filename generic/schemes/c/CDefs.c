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
#include  "CDefs.h"

void
_CcsAssertFailed_(const char * vstr, const char * fname, int line)
{
    fprintf(stderr, "Assert %s failed in %s#%d.\n", vstr, fname, line);
    exit(-1);
}

void *
_CcsMalloc_(size_t size, const char * fname, int line)
{
#ifdef NDEBUG
    return malloc(size);
#else
    void * ptr = malloc(size);
    memset(ptr, 0xA3, size);
    return ptr;
#endif
}

void *
_CcsRealloc_(void * ptr, size_t size, const char * fname, int line)
{
    return realloc(ptr, size);
}

void
_CcsFree_(void * ptr, const char * fname, int line)
{
    free(ptr);
}

char *
_CcsStrdup_(const char * str, const char * fname, int line)
{
    return strdup(str);
}

int
CcsUTF8GetCh(const char ** str, const char * stop)
{
    int ch, c1, c2, c3, c4;
    const char * cur = *str;

    if (*str == stop) return EoF;
    ch = *cur++;
    if (ch >= 128 && ((ch & 0xC0) != 0xC0) && (ch != EoF)) {
	fprintf(stderr, "Inside UTF-8 character!\n");
	exit(-1);
    }
    if (ch < 128 || ch == EoF) { *str = cur; return ch; }
    if ((ch & 0xF0) == 0xF0) {
	/* 1110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	c1 = ch & 0x07;
	if (cur >= stop || *cur == 0) goto broken;
	ch = *cur++;
	c2 = ch & 0x3F;
	if (cur >= stop || *cur == 0) goto broken;
	ch = *cur++;
	c3 = ch & 0x3F;
	if (cur >= stop || *cur == 0) goto broken;
	ch = *cur++;
	c4 = ch & 0x3F;
	*str = cur;
	return (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
    }
    if ((ch & 0xE0) == 0xE0) {
	/* 1110xxxx 10xxxxxx 10xxxxxx */
	c1 = ch & 0x0F;
	if (cur >= stop || *cur == 0) goto broken;
	ch = *cur++;
	c2 = ch & 0x3F;
	if (cur >= stop || *cur == 0) goto broken;
	ch = *cur++;
	c3 = ch & 0x3F;
	*str = cur;
	return (((c1 << 6) | c2) << 6) | c3;
    }
    /* (ch & 0xC0) == 0xC0 */
    /* 110xxxxx 10xxxxxx */
    c1 = ch & 0x1F;
    if (cur >= stop || *cur == 0) goto broken;
    ch = *cur++;
    c2 = ch & 0x3F;
    *str = cur;
    return (c1 << 6) | c2;
 broken:
    fprintf(stderr, "Broken in UTF8 character.\n");
    exit(-1);
}

int
CcsUTF8GetWidth(const char * str, size_t len)
{
    int ch, width;
    const char * stop = str + len;

    width = 0;
    while (str < stop) {
	ch = CcsUTF8GetCh(&str, stop);
	if (ch == 0 || ch == EoF) break;
	if (ch == '\t') width = (width + 8) - (width & 0x07);
	else if (ch < 32) /* Do Nothing */;
	else if (ch < 128) ++width;
 	/* All multi-byte characters are treated as 2 char width */
	else width += 2;
    }
    return width;
}

int
CcsUnescapeCh(const char ** str, const char * stop)
{
    int val;
    const char * cur = *str;

    if (cur >= stop) return EoF;
    if (*cur++ != '\\') { *str = cur; return *cur; }
    if (cur >= stop) return ErrorChr;
    switch (*cur) {
    case 'a': *str = cur + 1; return '\a';
    case 'b': *str = cur + 1; return '\b';
	/* Not all platform support \e */
	/*case 'e': *str = cur + 1; return '\e';*/
    case 'f': *str = cur + 1; return '\f';
    case 'n': *str = cur + 1; return '\n';
    case 'r': *str = cur + 1; return '\r';
    case 't': *str = cur + 1; return '\t';
    case 'v': *str = cur + 1; return '\v';
    case '\\': *str = cur + 1; return '\\';
    case '\'': *str = cur + 1; return '\'';
    case '\"': *str = cur + 1; return '\"';
    case '0': case '1': case '2': case '3': /* \nnn */
	if (cur + 3 >= stop) return ErrorChr;
	if (cur[1] < '0' || cur[1] > '7') return ErrorChr;
	if (cur[2] < '0' || cur[2] > '7') return ErrorChr;
	*str = cur + 3;
	return ((cur[0] - '0') << 6) | ((cur[1] - '0') << 3) | (cur[0] - '0');
    case 'x': /* \HH */
	if (cur + 3 >= stop) return ErrorChr;
	val = 0;
	if (cur[1] >= '0' && cur[1] <= '9') val = ((cur[1] - '0') << 4);
	else if (cur[1] >= 'A' && cur[1] <= 'F') val = ((cur[1] - 'A' + 10) << 4);
	else if (cur[1] >= 'a' && cur[1] <= 'f') val = ((cur[1] - 'a' + 10) << 4);
	else return ErrorChr;
	if (cur[2] >= '0' && cur[2] <= '9') val |= cur[2] - '0';
	else if (cur[2] >= 'A' && cur[2] <= 'F') val |= cur[2] - 'A' + 10;
	else if (cur[2] >= 'a' && cur[2] <= 'f') val |= cur[2] - 'a' + 10;
	else return ErrorChr;
	*str = cur + 3;
	return val;
    default:
	break;
    }
    return ErrorChr;
}

char *
CcsUnescape(const char * str, char stripCh)
{
    const char * cursrc, * stop; int ch;
    char * curtgt, * retval = CcsMalloc(strlen(str) + 1);

    if (!retval) return NULL;
    cursrc = str; curtgt = retval; stop = str + strlen(str);
    if (*cursrc == stripCh) ++cursrc;
    while (*cursrc) {
	if (cursrc[0] == stripCh && cursrc[1] == 0) break;
	if ((ch = CcsUnescapeCh(&cursrc, stop)) == ErrorChr) goto errquit;
	*curtgt++ = (char)ch;
    }
    *curtgt = 0;
    return retval;
 errquit:
    CcsFree(retval);
    return NULL;
}
