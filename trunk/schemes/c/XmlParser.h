/*---- license ----*/
/*-------------------------------------------------------------------------
 Coco.ATG -- Attributed Grammar
 Compiler Generator Coco/R,
 Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
 extended by M. Loeberbauer & A. Woess, Univ. of Linz
 with improvements by Pat Terry, Rhodes University.
 ported to C by Charles Wang <charlesw123456@gmail.com>

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
/*---- enable ----*/
#ifndef  COCO_XMLPARSER_H
#define  COCO_XMLPARSER_H

#ifndef  COCO_ERRORPOOL_H
#include "c/ErrorPool.h"
#endif

#ifndef  COCO_XMLSCANNER_H
#include "c/XmlScanner.h"
#endif

/*---- hIncludes ----*/
#ifndef  COCO_GLOBALS_H
#include  "Globals.h"
#endif
/*---- enable ----*/

EXTC_BEGIN

struct CcsXmlParser_s {
    CcsErrorPool_t    errpool;
    CcsXmlScanner_t   scanner;
    CcsToken_t      * t;
    CcsToken_t      * la;
    int               maxT;
    /*---- members ----*/
    CcGlobals_t       globals;
    CcsPosition_t   * members;
    CcsPosition_t   * constructor;
    CcsPosition_t   * destructor;
    /* Shortcut pointers */
    CcSymbolTable_t * symtab;
    CcXmlSpecMap_t  * xmlspecmap;
    CcSyntax_t      * syntax;
    /*---- enable ----*/
};

CcsXmlParser_t *
CcsXmlParser(CcsXmlParser_t * self, const char * fname, FILE * errfp);
void CcsXmlParser_Destruct(CcsXmlParser_t * self);
void CcsXmlParser_Parse(CcsXmlParser_t * self);

void
CcsXmlParser_SemErr(CcsXmlParser_t * self, const CcsToken_t * token,
		    const char * format, ...);
void
CcsXmlParser_SemErrT(CcsXmlParser_t * self, const char * format, ...);

EXTC_END

#endif /* COCO_XMLPARSER_H */
