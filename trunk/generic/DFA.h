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
#ifndef  COCO_DFA_H
#define  COCO_DFA_H

#include <stdio.h>

#ifndef  COCO_CHARSET_H
#include "CharSet.h"
#endif

#ifndef  COCO_PARSER_H
#include "Parser.h"
#endif

EXTC_BEGIN

typedef struct DFA_s DFA_t;
struct DFA_s {
    /* Private members. */
    int         maxStates;
    int         lastStateNr;
    State_t   * firstState;
    State_t   * lastState;
    int         lastSimState;
    FILE      * fram;
    FILE      * gen;
    Symbol_t  * curSy;
    Node_t    * curGraph;
    Bool_t      ignoreCase;
    Bool_t      dirtyDFA;
    Bool_t      hasCtxMoves;
    Parser_t  * parser;
    Tab_t     * tab;
    Errors_t  * errors;
    FILE      * trace;

    Melted_t  * firstMelted;
    Comment_t * firstComment;
};

DFA_t * DFA(DFA_t * self, Parser_t * parser);

void DFA_ConvertToStates(DFA_t * self, Node_t * p, Symbol_t * sym);
void DFA_MatchLiteral(DFA_t * self, const char * s, Symbol_t * sym);
void DFA_MakeDeterministic(DFA_t * self);
void DFA_PrintStates(DFA_t * self);
Action_t * DFA_FindAction(DFA_t * self, State_t * state, int ch);
void DFA_GetTargetStates(DFA_t * self, Action_t * a, BitArray_t ** targets,
			 Symbol_t ** endOf, Bool_t * ctx);

Melted_t * DFA_NewMelted(DFA_t * self, BitArray_t * set, State_t * state);
BitArray_t * DFA_MeltedSet(DFA_t * self, int nr);
Melted_t * DFA_StateWithSet(DFA_t * self, BitArray_t * s);
void DFA_NewComment(DFA_t * self, Node_t * from, Node_t * to, Bool_t nested);
void DFA_WriteScanner(DFA_t * self);

EXTC_END

#endif  /* COCO_DFA_H */
