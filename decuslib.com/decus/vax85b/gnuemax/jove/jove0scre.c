Y
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
 
   jove_screen.c 
 
   Deals with writing output to the screen optimally i.e. it doesn't 
   write what is already there.  It keeps an exact image of the screen 
   in the Screen array.  */ 
 
#include "jove.h" 
#include "jove_temp.h" 
#include "termcap.h" 
 
extern int	BufSize; 
 
int	CheckTime, 
	tabstop = 8; 
 
int	(*TTins_line)(), 
	(*TTdel_line)(); 
 
struct scrimage 
	*nimage,	/* What lines should be where after redisplay */ 
	*oimage;	/* What line are after redisplay */ 
 
struct screenline	*Screen,	/* The screen */ 
			*Curline;	/* Current Line */ 
char	*cursor,			/* Offset into current Line */ 
	*cursend; 
 
int	CapCol, 
	CapLine, 
 
	i_line, 
	i_col; 
 
make_scr() 
{ 
	register int	i; 
	register struct screenline	*ns; 
	register char	*nsp; 
 
	nimage = (struct scrimage *) emalloc(LI * sizeof (struct scrimage)); 
	oimage = (struct scrimage *) emalloc(LI * sizeof (struct scrimage)); 
 
	ns = Screen = (struct screenline *) 
			emalloc(LI * sizeof(struct screenline)); 
 
	nsp = (char *) emalloc(CO * LI); 
 
	for (i = 0; i < LI; i++) { 
		ns->s_line = nsp; 
		nsp += CO; 
		ns->s_length = nsp - 1;		/* End of Line */ 
		ns++; 
	} 
	cl_scr(); 
} 
 
clrline(cp1, cp2) 
register char	*cp1, 
		*cp2; 
{ 
	while (cp1 <= cp2) 
		*cp1++ = ' '; 
} 
 
cl_eol() 
{ 
	if (InputPending || cursor > cursend) 
		return; 
 
	if (cursor < Curline->s_length) { 
		clrline(cursor, Curline->s_length); 
		Curline->s_length = cursor; 
		Placur(i_line, i_col); 
		putpad(CE, 1); 
	} 
} 
 
cl_scr() 
{ 
	register int	i; 
	register struct screenline	*sp = Screen; 
 
	for (i = 0; i < LI; i++, sp++) { 
		clrline(sp->s_line, sp->s_length); 
		sp->s_length = sp->s_line; 
		oimage[i].Line = 0; 
	} 
	putpad(CL, LI); 
	Placur(0, 0); 
	UpdMesg++; 
} 
 
#define soutputc(c)	if (--n <= 0) break; else sputc(c) 
 
/* Output one character (if necessary) at the current position */ 
 
#define sputc(c)	((*cursor != c) ? dosputc(c) : (cursor++, i_col++)) 
 
dosputc(c) 
register char	c; 
{ 
	if (*cursor != c) { 
		Placur(i_line, i_col); 
		if (UL && c == '_' && *cursor != ' ') 
			putstr(" \b");		/* Erase so '_' looks right */ 
		outchar(c); 
		CapCol++; 
		*cursor++ = c; 
		i_col++; 
	} else { 
		cursor++; 
		i_col++; 
	} 
} 
 
/* Write `line' at the current position of `cursor'.  Stop when we 
   reach the end of the screen.  Aborts if there is a character 
   waiting.  */ 
 
swrite(line) 
register char	*line; 
{ 
	register char	c; 
	int	col = 0, 
		aborted = 0; 
	register int	n = cursend - cursor; 
 
	while (c = *line++) { 
		if (CheckTime) { 
			flusho(); 
			CheckTime = 0; 
			if (InputPending = charp()) { 
				aborted = 1; 
				break; 
			} 
		} 
		if (c == '\t') { 
			int	nchars; 
 
			nchars = (tabstop - (col % tabstop)); 
			col += nchars; 
 
			while (nchars--) 
				soutputc(' '); 
			if (n <= 0) 
				break; 
		} else if (c < 040 || c == '\177') { 
			soutputc('^'); 
			soutputc((c == '\177') ? '?' : c + '@'); 
			col += 2; 
		} else { 
			soutputc(c); 
			col++; 
		} 
	} 
	if (n <= 0) 
		sputc('!'); 
	if (cursor > Curline->s_length) 
		Curline->s_length = cursor; 
	return !aborted; 
} 
 
/* This is for writing a buffer line to the screen.  This is to 
   minimize the amount of copying from one buffer to another buffer. 
   This gets the info directly from ibuff1 or ibuff2. */ 
 
BufSwrite(linenum) 
{ 
	char	*bp; 
	Line	*lp = nimage[linenum].Line; 
	register int	n = cursend - cursor, 
			col = 0, 
			c; 
	int	tl = lp->l_dline, 
		nl, 
		StartCol = nimage[linenum].StartCol, 
		aborted = 0; 
 
	if (lp == curline) { 
		bp = linebuf; 
		nl = BUFSIZ; 
	} else { 
		if ((tl | DIRTY) == NullLine) 
			return 1;	/* Nothing to do! (Short line.) */ 
 
		bp = getblock(tl, READ); 
		nl = nleft; 
		tl &= ~OFFMSK; 
	} 
 
	if (*bp) for (;;) { 
		if (col >= StartCol) 
			break; 
 
		c = *bp++; 
		if (c == '\177') 
			col++, c = '?'; 
		if (c >= ' ') 
			col++; 
		else if (c == '\t') 
			col += (tabstop - (col % tabstop)); 
		else 
			col += 2; 
 
		if (--nl == 0) { 
			bp = getblock(tl += INCRMT, READ); 
			nl = nleft; 
		} 
	} 
 
	while (c = *bp++) { 
		if (CheckTime) { 
			flusho(); 
			CheckTime = 0; 
			if (InputPending = charp()) { 
				aborted = 1; 
				break; 
			} 
		} 
		if (c == '\177') { 
			soutputc('^'); 
			c = '?'; 
			col++; 
		} 
		if (c >= ' ') { 
			soutputc(c); 
			col++; 
		} else if (c == '\t') { 
			int	nchars = (tabstop - (col % tabstop)); 
 
			col += nchars; 
			while (--nchars >= 0) 
				soutputc(' '); 
			if (n <= 0) 
				break; 
		} else { 
			soutputc('^'); 
			col++; 
			soutputc(c + '@'); 
			col++; 
		} 
 
		if (--nl == 0) { 
			bp = getblock(tl += INCRMT, READ); 
			nl = nleft; 
		} 
	} 
	if (n <= 0) 
		sputc('!'); 
	if (cursor > Curline->s_length) 
		Curline->s_length = cursor; 
	return !aborted;		/* Didn't abort */ 
} 
 
i_set(nline, ncol) 
register int	nline, 
		ncol; 
{ 
	Curline = &Screen[nline]; 
	cursor = Curline->s_line + ncol; 
	cursend = &Curline->s_line[CO - 1]; 
	i_line = nline; 
	i_col = ncol; 
} 
 
extern int	line_diff; 
 
/* Insert `num' lines a top, but leave all the lines BELOW `bottom' 
   alone (at least they won't look any different when we are done). 
   This changes the screen array AND does the physical changes. */ 
 
v_ins_line(num, top, bottom) 
{ 
	register int	i; 
	struct screenline	savelines[MAXNLINES]; 
 
	/* Save the screen pointers. */ 
 
	for(i = 0; i < num && top + i <= bottom; i++) 
		savelines[i] = Screen[bottom - i]; 
 
	/* Num number of bottom lines will be lost. 
	   Copy everything down num number of times. */ 
 
	for (i = bottom; i > top && i-num >= 0; i--) 
		Screen[i] = Screen[i - num]; 
 
	/* Restore the saved ones, making them blank. */ 
 
	for (i = 0; i < num; i++) { 
		Screen[top + i] = savelines[i]; 
		clrline(Screen[top + i].s_line, Screen[top + i].s_length); 
	} 
 
	(*TTins_line)(top, bottom, num); 
} 
 
/* Delete `num' lines starting at `top' leaving the lines below `bottom' 
   alone.  This updates the internal image as well as the physical image.  */ 
 
v_del_line(num, top, bottom) 
{ 
	register int	i, 
			bot; 
	struct screenline	savelines[MAXNLINES]; 
 
	bot = bottom; 
 
	/* Save the lost lines. */ 
 
	for (i = 0; i < num && top + i <= bottom; i++) 
		savelines[i] = Screen[top + i]; 
 
	/* Copy everything up num number of lines. */ 
 
	for (i = top; num + i <= bottom; i++) 
		Screen[i] = Screen[i + num]; 
 
	/* Restore the lost ones, clearing them. */ 
 
	for (i = 0; i < num; i++) { 
		Screen[bottom - i] = savelines[i]; 
		clrline(Screen[bot].s_line, Screen[bot].s_length); 
		bot--; 
	} 
 
	(*TTdel_line)(top, bottom, num); 
} 
 
/* The cursor optimization happens here.  You may decide that this 
   is going too far with cursor optimization, or perhaps it should 
   limit the amount of checking to when the output speed is slow. 
   What ever turns you on ...   */ 
 
extern int	CapCol, 
		CapLine; 
 
struct cursaddr { 
	int	c_numchars, 
		(*c_func)(); 
}; 
 
char	*Cmstr; 
struct cursaddr	*HorMin, 
		*VertMin, 
		*DirectMin; 
 
#define	FORWARD		0	/* Move forward */ 
#define FORTAB		1	/* Forward using tabs */ 
#define	BACKWARD	2	/* Move backward */ 
#define RETFORWARD	3	/* Beginning of line and then tabs */ 
#define NUMHOR		4 
 
#define DOWN		0	/* Move down */ 
#define UPMOVE		1	/* Move up */ 
#define NUMVERT		2 
 
#define DIRECT		0	/* Using CM */ 
#define HOME		1	/* HOME	*/ 
#define LOWER		2	/* Lower Line */ 
#define NUMDIRECT	3 
 
#define	home()		Placur(0, 0) 
#define LowLine()	putpad(LL, 1), CapLine = LI - 1, CapCol = 0 
#define PrintHo()	putpad(HO, 1), CapLine = CapCol = 0 
 
struct cursaddr	WarpHor[NUMHOR], 
		WarpVert[NUMVERT], 
		WarpDirect[NUMDIRECT]; 
 
int	phystab = 8; 
 
GoDirect(line, col) 
register int	line, 
		col; 
{ 
	putpad(Cmstr, 1), CapLine = line, CapCol = col; 
} 
 
RetTab(col) 
register int	col; 
{ 
	outchar('\r'), CapCol = 0, ForTab(col); 
} 
 
HomeGo(line, col) 
{ 
	PrintHo(), DownMotion(line), ForTab(col); 
} 
 
BottomUp(line, col) 
register int	line, 
		col; 
{ 
	LowLine(), UpMotion(line), ForTab(col); 
} 
 
/* Tries to move forward using tabs (if possible).  It tabs to the 
   closest tabstop which means it may go past 'destcol' and backspace 
   to it. */ 
 
ForTab(destcol) 
int	destcol; 
{ 
	register int	tabgoal, 
			ntabs, 
			tabstp = phystab; 
 
	if (TABS) { 
		tabgoal = destcol + (tabstp / 2); 
		tabgoal -= (tabgoal % tabstp); 
 
		/* Don't tab to last place or else it is likely to screw up */ 
		if (tabgoal >= CO) 
			tabgoal -= tabstp; 
 
		ntabs = (tabgoal / tabstp) - (CapCol / tabstp); 
		while (--ntabs >= 0) 
			outchar('\t'); 
		CapCol = tabgoal; 
	} 
	if (CapCol > destcol) 
		BackMotion(destcol); 
	else if (CapCol < destcol) 
		ForMotion(destcol); 
} 
 
extern struct screenline	*Screen; 
 
ForMotion(destcol) 
register int	destcol; 
{ 
	register int	nchars = destcol - CapCol; 
	register char	*cp = &Screen[CapLine].s_line[CapCol]; 
 
	while (--nchars >= 0) 
		outchar(*cp++); 
	CapCol = destcol; 
} 
 
BackMotion(destcol) 
register int	destcol; 
{ 
	register int	nchars = CapCol - destcol; 
 
	if (BC) 
		while (--nchars >= 0) 
			putpad(BC, 1); 
	else 
		while (--nchars >= 0) 
			outchar('\b'); 
	CapCol = destcol; 
} 
 
DownMotion(destline) 
register int	destline; 
{ 
	register int	nlines = destline - CapLine; 
 
	while (--nlines >= 0) 
		outchar('\n'); 
	CapLine = destline; 
} 
 
UpMotion(destline) 
register int	destline; 
{ 
	register int	nchars = CapLine - destline; 
 
	while (--nchars >= 0) 
		putpad(UP, 1); 
	CapLine = destline; 
} 
 
InitCM() 
{ 
	WarpHor[FORWARD].c_func = ForMotion; 
	WarpHor[BACKWARD].c_func = BackMotion; 
	WarpHor[FORTAB].c_func = ForTab; 
	WarpHor[RETFORWARD].c_func = RetTab; 
 
	WarpVert[DOWN].c_func = DownMotion; 
	WarpVert[UPMOVE].c_func = UpMotion; 
 
	WarpDirect[DIRECT].c_func = GoDirect; 
	WarpDirect[HOME].c_func = HomeGo; 
	WarpDirect[LOWER].c_func = BottomUp; 
 
	HomeLen = HO ? strlen(HO) : 1000; 
	LowerLen = LL ? strlen(LL) : 1000; 
	UpLen = UP ? strlen(UP) : 1000; 
} 
 
DoPlacur(line, col) 
{ 
	int	dline,		/* Number of lines to move */ 
		dcol;		/* Number of columns to move */ 
	register int	best, 
			i; 
	register struct cursaddr	*cp; 
#ifdef ID_CHAR 
	extern int	InMode;		/* Terminal is in insert-mode */ 
#endif 
 
#define CursMin(which,addrs,max) \ 
	for (best = 0, cp = &addrs[1], i = 1; i < max; i++, cp++) \ 
		if (cp->c_numchars < addrs[best].c_numchars) \ 
			best = i; \ 
	which = &addrs[best]; 
 
	if (line == CapLine && col == CapCol) 
		return;		/* We are already there. */ 
 
	dline = line - CapLine; 
	dcol = col - CapCol; 
 
	/* Number of characters to move horizontally for each case. 
	 * 1: Just move forward by typing the right character on the screen 
	 * 2: Print the correct number of back spaces 
	 * 3: Try tabbing to the correct place 
	 * 4: Try going to the beginning of the line, and then tab 
	 */ 
 
	if (dcol == 1 || dcol == 0) {		/* Most common case */ 
		HorMin = &WarpHor[FORWARD]; 
		HorMin->c_numchars = dcol; 
	} else { 
		WarpHor[FORWARD].c_numchars = dcol >= 0 ? dcol : 1000; 
		WarpHor[BACKWARD].c_numchars = dcol < 0 ? -dcol : 1000; 
		WarpHor[FORTAB].c_numchars = dcol >= 0 && TABS ? 
				ForNum(CapCol, col) : 1000; 
		WarpHor[RETFORWARD].c_numchars = (1 + (TABS ? ForNum(0, col) : col)); 
 
		/* Which is the shortest of the bunch */ 
 
		CursMin(HorMin, WarpHor, NUMHOR); 
	} 
 
	/* Moving vertically is more simple. */ 
 
	WarpVert[DOWN].c_numchars = dline >= 0 ? dline : 1000; 
	WarpVert[UPMOVE].c_numchars = dline < 0 ? ((-dline) * UpLen) : 1000; 
 
	/* Which of these is simpler */ 
	CursMin(VertMin, WarpVert, NUMVERT); 
 
	/* Homing first and lowering first are considered  
	   direct motions. 
	   Homing first's total is the sum of the cost of homing 
	   and the sum of tabbing (if possible) to the right. */ 
	 
	if (VertMin->c_numchars + HorMin->c_numchars <= 3) { 
		DirectMin = &WarpDirect[DIRECT];	/* A dummy ... */ 
		DirectMin->c_numchars = 100; 
	} else { 
		WarpDirect[DIRECT].c_numchars = CM ? 
				strlen(Cmstr = tgoto(CM, col, line)) : 1000; 
		WarpDirect[HOME].c_numchars = HomeLen + line + 
				WarpHor[RETFORWARD].c_numchars; 
		WarpDirect[LOWER].c_numchars = LowerLen + ((LI - line - 1) * UpLen) + 
				WarpHor[RETFORWARD].c_numchars; 
		CursMin(DirectMin, WarpDirect, NUMDIRECT); 
	} 
 
	if (HorMin->c_numchars + VertMin->c_numchars < DirectMin->c_numchars) { 
		if (line != CapLine) 
			(*VertMin->c_func)(line); 
		if (col != CapCol) { 
#ifdef ID_CHAR 
			if (InMode)	/* We may use real characters ... */ 
				putstr(EI), InMode = 0; 
#endif 
			(*HorMin->c_func)(col); 
		} 
	} else { 
#ifdef ID_CHAR 
		if (InMode && !MI) 
			putstr(EI), InMode = 0; 
#endif 
		(*DirectMin->c_func)(line, col); 
	} 
} 
 
#define abs(x)	((x) >= 0 ? (x) : -(x)) 
 
ForNum(from, to) 
register int	from; 
{ 
	register int	tabgoal, 
			tabstp = phystab; 
	int		numchars = 0; 
 
	if (from > to) 
		return from - to; 
	if (TABS) { 
		tabgoal = to + (tabstp / 2); 
		tabgoal -= (tabgoal % tabstp); 
		if (tabgoal >= CO) 
			tabgoal -= tabstp; 
		numchars = (tabgoal / tabstop) - (from / tabstp); 
		from = tabgoal; 
	} 
	return numchars + abs(from - to); 
} 
 
#ifdef WIRED_TERMS 
 
BGi_lines(top, bottom, num) 
{ 
	printf("\033[?6h\033[%d;%dr\033[%dL\033[r", top + 1, bottom + 1, num); /* fhsu */ 
	CapCol = CapLine = 0; 
} 
 
SUNi_lines(top, bottom, num) 
{ 
	Placur(bottom - num + 1, 0); 
	printf("\033[%dM", num); 
	Placur(top, 0); 
	printf("\033[%dL", num); 
} 
 
C100i_lines(top, bottom, num) 
{ 
	if (num <= 1) { 
		GENi_lines(top, bottom, num); 
		return; 
	} 
	printf("\033v%c%c%c%c", ' ', ' ', ' ' + bottom + 1, ' ' + CO); 
	CapLine = CapCol = 0; 
	Placur(top, 0); 
	while (num--) 
		putpad(AL, LI - CapLine); 
	printf("\033v%c%c%c%c", ' ', ' ', ' ' + LI, ' ' + CO); 
	CapLine = CapCol = 0; 
} 
 
#endif WIRED_TERMS 
 
GENi_lines(top, bottom, num) 
{ 
	register int	i; 
 
	if (CS) { 
		printf(tgoto(CS, bottom, top)); 
		CapCol = CapLine = 0; 
		Placur(top, 0); 
		for (i = 0; i < num; i++) 
			putpad(SR, 0); 
		printf(tgoto(CS, LI - 1, 0)); 
		CapCol = CapLine = 0; 
	} else { 
		Placur(bottom - num + 1, 0); 
		for (i = 0; i < num; i++) 
			putpad(DL, LI - CapLine); 
		Placur(top, 0); 
		for (i = 0; i < num; i++) 
			putpad(AL, LI - CapLine); 
	} 
} 
 
#ifdef WIRED_TERMS 
 
BGd_lines(top, bottom, num) 
{ 
	printf("\033[?6h\033[%d;%dr\033[%dM\033[r", top + 1, bottom + 1, num); /* fhsu */ 
	CapCol = CapLine = 0; 
} 
 
SUNd_lines(top, bottom, num) 
{ 
	Placur(top, 0); 
	printf("\033[%dM", num); 
	Placur(bottom + 1 - num, 0); 
	printf("\033[%dL", num); 
} 
 
C100d_lines(top, bottom, num) 
{ 
	if (num <= 1) { 
		GENd_lines(top, bottom, num); 
		return; 
	} 
	printf("\033v%c%c%c%c", ' ', ' ', ' ' + bottom + 1, ' ' + CO); 
	CapLine = CapCol = 0; 
	Placur(top, 0); 
	while (num--) 
		putpad(DL, LI - CapLine); 
	printf("\033v%c%c%c%c", ' ', ' ', ' ' + LI, ' ' + CO); 
	CapLine = CapCol = 0; 
} 
 
#endif WIRED_TERMS 
 
GENd_lines(top, bottom, num) 
{ 
	register int	i; 
 
	if (CS) { 
		printf(tgoto(CS, bottom, top)); 
		CapCol = CapLine = 0; 
		Placur(bottom, 0); 
		for (i = 0; i < num; i++) 
			outchar('\n'); 
		printf(tgoto(CS, LI - 1, 0)); 
		CapCol = CapLine = 0; 
	} else { 
		Placur(top, 0); 
		for (i = 0; i < num; i++) 
			putpad(DL, LI - CapLine); 
		Placur(bottom + 1 - num, 0); 
		for (i = 0; i < num; i++) 
			putpad(AL, LI - CapLine); 
	} 
} 
 
struct ID_lookup { 
	char	*ID_name; 
	int	(*I_func)();	/* Function to insert lines */ 
	int	(*D_func)();	/* Function to delete lines */ 
} ID_trms[] = { 
	"generic",	GENi_lines,	GENd_lines,	/* This should stay here */ 
#ifdef WIRED_TERMS 
	"sun",		SUNi_lines,	SUNd_lines, 
	"bg",		BGi_lines,	BGd_lines, 
	"c1",		C100i_lines,	C100d_lines, 
#endif WIRED_TERMS 
	0,		0,		0 
}; 
 
IDline_setup(tname) 
char	*tname; 
{ 
	register struct ID_lookup	*idp; 
 
	for (idp = &ID_trms[1]; idp->ID_name; idp++) 
		if (strncmp(idp->ID_name, tname, strlen(idp->ID_name)) == 0) 
			break; 
	if (idp->ID_name == 0) 
		idp = &ID_trms[0]; 
	TTins_line = idp->I_func; 
	TTdel_line = idp->D_func; 
} 
 
