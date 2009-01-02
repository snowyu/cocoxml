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
-------------------------------------------------------------------------*/
#include  "pgngame.h"

typedef struct {
    int castlingRow;
    int passRow;
    PgnPiece_t king;
    PgnPiece_t queen;
    PgnPiece_t rook;
    PgnPiece_t bishop;
    PgnPiece_t knight;
    PgnPiece_t pawn;
}  PgnInfo_t;
static const PgnInfo_t whiteInfo = {
    0, 4, wKing, wQueen, wRook, wBishop, wKnight, wPawn
};
static const PgnInfo_t blackInfo = {
    7, 3, bKing, bQueen, bRook, bBishop, bKnight, bPawn
};

static int
PgnPiece2Material(PgnPiece_t piece)
{
    switch (piece) {
    case wQueen: case bQueen: return 9;
    case wRook: case bRook: return 5;
    case wBishop: case bBishop: return 3;
    case wKnight: case bKnight: return 3;
    case wPawn: case bPawn: return 1;
    default: break;
    }
    return 0;
}

void
PgnGameStatus_Clone(PgnGameStatus_t * self, const PgnGameStatus_t * status)
{
    memcpy(self, status, sizeof(*self));
}

const PgnGameStatus_t PgnStandardStart = {
    { "RNBQKBNR",
      "PPPPPPPP",
      "........",
      "........",
      "........",
      "........",
      "pppppppp",
      "rnbqkbnr" },
    { 39, TRUE, TRUE },
    { 39, TRUE, TRUE }
};

PgnMove_t *
PgnMove(CcsBool_t WhiteOrNot, const char * value)
{
    PgnMove_t * self;
    char * curtgt; const char * cursrc; size_t valuelen;
    const PgnInfo_t * info = WhiteOrNot ? &whiteInfo : &blackInfo;

    if (!(self = CcsMalloc(sizeof(PgnMove_t) + strlen(value) + 2)))
	return NULL;
    memset(self, 0, sizeof(PgnMove_t));
    self->WhiteOrNot = WhiteOrNot;

    curtgt = (char *)(self + 1); cursrc = value;
    valuelen = strspn(value, "KQRNBabcdefgh12345678xO-");
    self->value = curtgt;
    memcpy(curtgt, cursrc, valuelen);
    curtgt[valuelen] = 0;
    curtgt += valuelen + 1; cursrc += valuelen;

    self->upgrade = pgnBlank;
    if (cursrc[0] == '=') {
	switch (cursrc[1]) {
	case 'Q': self->upgrade = info->queen; cursrc += 2; break;
	case 'R': self->upgrade = info->rook; cursrc += 2; break;
	case 'B': self->upgrade = info->bishop; cursrc += 2; break;
	case 'N': self->upgrade = info->knight; cursrc += 2; break;
	}
    }

    self->annotation = curtgt;
    strcpy(curtgt, cursrc);
    return self;
}

void
PgnMove_Destruct(PgnMove_t * self)
{
    CcsFree(self);
}

static size_t safe_strlen(const char * str)
{
    return str ? strlen(str) + 1: 0;
}
static char * safe_append(char ** cur, const char * str)
{
    char * ret;
    if (!str) return NULL;
    ret = *cur;
    strcpy(*cur, str); *cur += strlen(str) + 1;
    return ret;
}

PgnGame_t *
PgnGame(const PgnGameStatus_t * status,
	const char * Event, const char * Site, const char * Date,
	const char * Round, const char * White, const char * Black,
	const char * WhiteElo, const char * BlackElo, const char * TimeControl)
{
    PgnGame_t * self;  char * cur;
    size_t len = safe_strlen(Event) + safe_strlen(Site) + safe_strlen(Date) +
	safe_strlen(Round) + safe_strlen(White) + safe_strlen(Black) +
	safe_strlen(TimeControl);
    if (!(self = CcsMalloc(sizeof(PgnGame_t) + len))) return NULL;
    self->next = NULL;

    cur = (char *)(self + 1);
    self->Event = safe_append(&cur, Event);
    self->Site = safe_append(&cur, Site);
    self->Date = safe_append(&cur, Date);
    self->Round = safe_append(&cur, Round);
    self->White = safe_append(&cur, White);
    self->Black = safe_append(&cur, Black);
    self->WhiteElo = WhiteElo && *WhiteElo == '"' ? atoi(WhiteElo + 1) : -1;
    self->BlackElo = BlackElo && *BlackElo == '"' ? atoi(BlackElo + 1) : -1;
    self->TimeControl = safe_append(&cur, TimeControl);
    self->Result = NULL;
    self->resultInfo = NULL;

    PgnGameStatus_Clone(&self->startStatus, status);
    PgnGameStatus_Clone(&self->status, status);

    self->movesArr.prev = NULL;
    self->movesArr.next = NULL;
    self->movesArrLast = &self->movesArr;
    self->moveCur = self->moveLast = self->movesArrLast->moves;
    return self;
}

void
PgnGame_Destruct(PgnGame_t * self)
{
    PgnMovesArr_t * curarr, * nextarr;
    PgnMove_t ** cur, ** last;

    for (curarr = &self->movesArr; curarr; curarr = nextarr) {
	nextarr = curarr->next;
	last = nextarr ? curarr->moves + SZ_MOVES_ARR : self->moveLast;
	for (cur = curarr->moves; cur < last; ++cur)
	    PgnMove_Destruct(*cur);
	if (curarr != &self->movesArr) CcsFree(curarr);
    }
    if (self->resultInfo) CcsFree(self->resultInfo);
    if (self->Result) CcsFree(self->Result);
    CcsFree(self);
}

static const char * Xstr = "abcdefgh";
static const char * Ystr = "12345678";
static CcsBool_t
CheckColor(PgnPiece_t piece, CcsBool_t WhiteOrNot)
{
    switch (piece) {
    case wKing: case wQueen: case wRook:
    case wBishop: case wKnight: case wPawn:
	return WhiteOrNot;
    case bKing: case bQueen: case bRook:
    case bBishop: case bKnight: case bPawn:
	return !WhiteOrNot;
    default: break;
    }
    return FALSE;
}

static CcsBool_t CheckKing(int dx, int dy)
{
    return dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1;
}
static CcsBool_t CheckQueen(int dx, int dy)
{
    return dx == 0 || dy == 0 || dx == dy || dx == -dy;
}
static CcsBool_t CheckRook(int dx, int dy)
{
    return dx == 0 || dy == 0;
}
static CcsBool_t CheckBishop(int dx, int dy)
{
    return dx == dy || dx == -dy;
}
static CcsBool_t CheckKnight(int dx, int dy)
{
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
}

static CcsBool_t
PgnGame_Search(const PgnGame_t * self, PgnMove_t * move, PgnPiece_t piece,
	       CcsBool_t (* checker)(int dx, int dy))
{
    CcsBool_t found;
    int fx, fx0, fx1, fy, fy0, fy1;
    int tx, tx0, tx1, ty, ty0, ty1;
    const PgnGameStatus_t * status = &self->status;

    found = FALSE;
    if (move->fx == -1) { fx0 = 0; fx1 = 7; }
    else { fx0 = fx1 = move->fx; }
    if (move->fy == -1) { fy0 = 0; fy1 = 7; }
    else { fy0 = fy1 = move->fy; }
    if (move->tx == -1) { tx0 = 0; tx1 = 7; }
    else { tx0 = tx1 = move->tx; }
    if (move->ty == -1) { ty0 = 0; ty1 = 7; }
    else { ty0 = ty1 = move->ty; }

    for (fy = fy0; fy <= fy1; ++fy)
	for (fx = fx0; fx <= fx1; ++fx) {
	    if (status->board[fy][fx] != piece) continue;
	    for (ty = ty0; ty <= ty1; ++ty)
		for (tx = tx0; tx <= tx0; ++tx) {
		    if (fx == fy && tx == ty) continue;
		    if (CheckColor(status->board[tx][ty], move->WhiteOrNot))
			continue;
		    if (!checker(tx - fx, ty - fy))
			continue;
		    if (found) return FALSE; /* Multiple possibility found. */
		    found = TRUE;
		    if (move->fx == -1) move->fx = fx;
		    if (move->fy == -1) move->fy = fy;
		    if (move->tx == -1) move->tx = tx;
		    if (move->ty == -1) move->tx = ty;
		}
	}
    return found;
}

static CcsBool_t
PgnGame_SearchPawn(const PgnGame_t * self, PgnMove_t * move)
{
}

static CcsBool_t
PgnGame_FillMove(const PgnGame_t * self, PgnMove_t * move)
{
    const char * curb, * cure, * c;
    const PgnGameStatus_t * status = &self->status;
    const PgnSide_t * side = move->WhiteOrNot ? &status->white : &status->black;
    const PgnInfo_t * info = move->WhiteOrNot ? &whiteInfo : &blackInfo;
    const PgnInfo_t * opinfo = move->WhiteOrNot ? &blackInfo : &whiteInfo;

    move->castling = side->castling; move->castlingL = side->castlingL;
    if (!strcmp(move->value, "O-O")) {
	if (!side->castling) return FALSE;
	move->fpiece = move->tpiece = info->king;
	move->fy = move->ty = info->castlingRow;
	move->fx = 4; move->tx = 6;
	move->kpiece = pgnBlank;
	return TRUE;
    } else if (!strcmp(move->value, "O-O-O")) {
	if (!side->castlingL) return FALSE;
	move->fpiece = move->tpiece = info->king;
	move->fy = move->ty = info->castlingRow;
	move->fx = 4; move->tx = 2;
	move->kpiece = pgnBlank;
	return TRUE;
    }
    /* Set move->fpiece, move->tpiece. */
    curb = move->value; cure = move->value + strlen(move->value) - 1;
    switch (*curb) {
    case 'K': move->fpiece = info->king; ++curb; break;
    case 'Q': move->fpiece = info->queen; ++curb; break;
    case 'R': move->fpiece = info->rook; ++curb; break;
    case 'B': move->fpiece = info->bishop; ++curb; break;
    case 'N': move->fpiece = info->knight; ++curb; break;
    default: move->fpiece = info->pawn; break;
    }
    move->tpiece = move->upgrade == pgnBlank ? move->fpiece : move->upgrade;
    /* Set move->fx, move->fy, move->tx, move->ty */
    move->fx = move->fy = move->tx = move->ty = -1;
    if (curb <= cure && (c = strchr(Ystr, *cure))) {
	move->ty = c - Ystr; --cure;
    }
    if (curb <= cure && (c = strchr(Xstr, *cure))) {
	move->tx = c - Xstr; --cure;
    }
    if (curb <= cure && (c = strchr(Xstr, *curb))) {
	move->fx = c - Xstr; ++curb;
    }
    if (curb <= cure && (c = strchr(Ystr, *curb))) {
	move->fy = c - Ystr; ++curb;
    }
    if (move->tx == -1 && move->ty == -1) return FALSE;
    if (move->fx == -1 || move->fy == -1 || move->tx == -1 || move->ty == -1) {
	if (move->fpiece == info->king) {
	    if (!PgnGame_Search(self, move, info->king, CheckKing))
		return FALSE;
	} else if (move->fpiece == info->queen) {
	    if (!PgnGame_Search(self, move, info->queen, CheckQueen))
		return FALSE;
	} else if (move->fpiece == info->rook) {
	    if (!PgnGame_Search(self, move, info->rook, CheckRook))
		return FALSE;
	} else if (move->fpiece == info->bishop) {
	    if (!PgnGame_Search(self, move, info->bishop, CheckBishop))
		return FALSE;
	} else if (move->fpiece == info->knight) {
	    if (!PgnGame_Search(self, move, info->knight, CheckKnight))
		return FALSE;
	} else if (move->fpiece == info->pawn) {
	    if (!PgnGame_SearchPawn(self, move))
		return FALSE;
	}
    }
    if (move->fx == -1 || move->fy == -1 || move->tx == -1 || move->ty == -1)
	return FALSE;
    /* Set move->kpiece, move->kx, move->ky */
    if (move->fpiece == info->pawn && move->fy == info->passRow &&
	move->fx != move->tx && status->board[move->ty][move->tx] == pgnBlank) {
	/* Pawn Pass detected. */
	if (status->board[move->fy][move->tx] == opinfo->pawn) return FALSE;
	move->kpiece = opinfo->pawn; move->kx = move->tx; move->ky = move->fy;
    } else {
	move->kpiece = status->board[move->ty][move->tx];
	move->kx = move->tx; move->ky = move->ty;
    }
    return TRUE;
}

CcsBool_t
PgnGame_AppendMove(PgnGame_t * self, PgnMove_t * move)
{
    PgnMovesArr_t * newMovesArr;

    PgnGame_ToEnd(self);
    if (!PgnGame_FillMove(self, move)) return FALSE;
    if (self->moveLast - self->movesArrLast->moves >= SZ_MOVES_ARR) {
	if (!(newMovesArr = CcsMalloc(sizeof(PgnMovesArr_t)))) return FALSE;
	newMovesArr->prev = self->movesArrLast;
	newMovesArr->next = NULL;
	self->movesArrLast->next = newMovesArr;
	self->movesArrLast = newMovesArr;
	self->moveLast = self->movesArrLast->moves;
    }
    *self->moveLast++ = move;
    PgnGame_Forward(self);
    return TRUE;
}

void
PgnGame_ToStart(PgnGame_t * self)
{
    PgnGameStatus_Clone(&self->status, &self->startStatus);
    self->movesArrCur = &self->movesArr;
    self->moveCur = self->movesArrCur->moves;
}
void
PgnGame_Backward(PgnGame_t * self)
{
    PgnMove_t * cur;
    PgnSide_t * side, * opside;
    const PgnInfo_t * info;
    PgnGameStatus_t * status = &self->status;

    if (self->moveCur == self->movesArr.moves) return;
    /* Backward cur */
    if (self->moveCur > self->movesArrCur->moves) {
	--self->moveCur;
    } else {
	CcsAssert(self->movesArrCur->prev != NULL);
	self->movesArrCur = self->movesArrCur->prev;
	self->moveCur = self->movesArrCur->moves + (SZ_MOVES_ARR - 1);
    }
    cur = *self->moveCur;
    side = cur->WhiteOrNot ? &status->white : &status->black;
    opside = cur->WhiteOrNot ? &status->black : &status->white;
    info = cur->WhiteOrNot ? &whiteInfo : &blackInfo;
    /* Basic moves */
    status->board[cur->fy][cur->fx] = cur->fpiece;
    status->board[cur->ty][cur->tx] = pgnBlank;
    if (cur->fpiece != cur->tpiece)
	side->material -=
	    PgnPiece2Material(cur->tpiece) - PgnPiece2Material(cur->fpiece);
    if (cur->kpiece != pgnBlank) {
	CcsAssert(status->board[cur->ky][cur->kx] == pgnBlank);
	status->board[cur->ky][cur->kx] = cur->kpiece;
	opside->material += PgnPiece2Material(cur->kpiece);
    }
    /* Deal with castling & castlingL */
    side->castling = cur->castling; side->castlingL = cur->castlingL;
    if (cur->fy == info->castlingRow && cur->ty == info->castlingRow &&
	cur->fpiece == info->king && cur->fx == 4) {
	if (cur->tx == 6) { /* castling */
	    CcsAssert(status->board[info->castlingRow][7] == info->rook);
	    CcsAssert(status->board[info->castlingRow][5] == pgnBlank);
	    status->board[info->castlingRow][7] = pgnBlank;
	    status->board[info->castlingRow][5] = info->rook;
	} else if (cur->ty == 2) { /* castlingL */
	    CcsAssert(status->board[info->castlingRow][0] == info->rook);
	    CcsAssert(status->board[info->castlingRow][1] == pgnBlank);
	    CcsAssert(status->board[info->castlingRow][3] == pgnBlank);
	    status->board[info->castlingRow][0] = pgnBlank;
	    status->board[info->castlingRow][3] = info->rook;
	}
    }
}

void
PgnGame_Forward(PgnGame_t * self)
{
    PgnGameStatus_t * status = &self->status;
    PgnMove_t * cur = *self->moveCur;
    PgnSide_t * side = cur->WhiteOrNot ? &status->white : &status->black;
    PgnSide_t * opside = cur->WhiteOrNot ? &status->black : &status->white;
    const PgnInfo_t * info = cur->WhiteOrNot ? &whiteInfo : &blackInfo;

    if (self->moveCur == self->moveLast) return;
    /* Basic moves */
    if (cur->kpiece != pgnBlank) {
	CcsAssert(status->board[cur->ky][cur->kx] == cur->kpiece);
	status->board[cur->ky][cur->kx] = pgnBlank;
	opside->material -= PgnPiece2Material(cur->kpiece);
    }
    status->board[cur->fy][cur->fx] = pgnBlank;
    status->board[cur->ty][cur->tx] = cur->tpiece;
    if (cur->fpiece != cur->tpiece)
	side->material +=
	    PgnPiece2Material(cur->tpiece) - PgnPiece2Material(cur->fpiece);
    /* Deal with castling & castlingL */
    CcsAssert(cur->castling == side->castling);
    CcsAssert(cur->castlingL == side->castlingL);
    if ((side->castling || side->castlingL) &&
	cur->fy == info->castlingRow) {
	if (cur->fpiece == info->king) {
	    if (cur->fx == 4 && cur->tx == 6) { /* Short castling */
		CcsAssert(side->castling);
		CcsAssert(status->board[info->castlingRow][0] == info->rook);
		CcsAssert(status->board[info->castlingRow][1] == pgnBlank);
		CcsAssert(status->board[info->castlingRow][3] == pgnBlank);
		status->board[info->castlingRow][0] = pgnBlank;
		status->board[info->castlingRow][3] = info->rook;
	    } else if (cur->fx == 4 && cur->tx == 2) { /* Long castling */
		CcsAssert(side->castlingL);
		CcsAssert(status->board[info->castlingRow][7] == info->rook);
		CcsAssert(status->board[info->castlingRow][5] == pgnBlank);
		status->board[info->castlingRow][7] = pgnBlank;
		status->board[info->castlingRow][5] = info->rook;
	    }
	    side->castling = side->castlingL = FALSE;
	} else if (cur->fpiece == info->rook) {
	    if (side->castling && cur->fx == 7) side->castling = FALSE;
	    else if (side->castlingL && cur->fx == 0) side->castlingL = FALSE;
	}
    }
    /* Forward cur */
    ++self->moveCur;
    if (self->moveCur >= self->movesArrCur->moves + SZ_MOVES_ARR) {
	CcsAssert(self->movesArrCur->next != NULL);
	self->movesArrCur = self->movesArrCur->next;
	self->moveCur = self->movesArrCur->moves;
    }
}

void
PgnGame_ToEnd(PgnGame_t * self)
{
    while (self->moveCur != self->moveLast) PgnGame_Forward(self);
}
