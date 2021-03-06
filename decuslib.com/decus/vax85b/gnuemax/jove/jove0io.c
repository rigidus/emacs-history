/*R
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
   
   Commands to read/write files/regions.  */ 
 
#include "jove.h" 
#include "termcap.h" 
#ifdef JOVE4.2 
#	include <sys/dir.h> 
#else 
#	include "dir.h" 
#endif 
#include "pwd.h" 
#ifdef PROCS 
#include <signal.h> 
#endif 
 
#include <sys/stat.h> 
 
extern char *getenv (); 
extern char *sys_errlist[]; 
extern errno; 
 
char	*WERROR	= "Write error"; 
long	count = 0; 
int	nlines = 0; 
 
char	iobuff[LBSIZE], 
	genbuf[LBSIZE], 
	linebuf[LBSIZE], 
	*nextip; 
int	ninbuf, 
	io = -1; 
 
disk_line	NullLine = -1; 
 
IOclose() 
{ 
	if (io > 0) 
		ignore(close(io)), io = -1; 
	nextip = iobuff; 
	ninbuf = 0; 
} 
 
/* This reads a line from the input file into buf. */ 
 
getfline(buf) 
char	*buf; 
{ 
	register int	c; 
	register char	*lp, 
			*fp; 
 
	lp = buf; 
	*lp = '\0'; 
	fp = nextip; 
	do { 
		if (--ninbuf < 0) { 
			ninbuf = read(io, iobuff, LBSIZE) - 1; 
			fp = iobuff; 
			if (ninbuf < 0) { 
				if (lp != buf) 
					add_mess(" [Incomplete last line]"); 
				*lp = '\0'; 
				IOclose(); 
				goto eof; 
			} 
		} 
		c = *fp++; 
		if (c == '\0') 
			continue; 
		if (lp >= &buf[LBSIZE - 1]) { 
			add_mess("[Line to long]"); 
			IOclose(); 
			goto eof; 
		} 
 
		*lp++ = c; 
		count++; 
	} while (c != '\n'); 
	*--lp = 0; 
	nextip = fp; 
	return 0; 
eof: 
	return EOF; 
} 
 
/* Write the region from line1/char1 to line2/char2 to the open 
   fildes `io'.  */ 
 
int	EndWNewline = 1; 
 
putreg(line1, char1, line2, char2) 
Line	*line1, 
	*line2; 
{ 
	int n; 
	register char *fp, *lp; 
	register int	nib; 
 
	count = nlines = 0; 
	nib = 512; 
	fp = iobuff; 
	lsave();	/* Need this! */ 
 
	while (line1 != line2->l_next) { 
		lp = getline(line1->l_dline, linebuf) + char1; 
		if (line1 == line2) 
			linebuf[char2] = '\0'; 
		for (;;) { 
			if (--nib < 0) { 
				n = fp - iobuff; 
				if (write(io, iobuff, n) != n) 
					goto werror; 
				nib = 511; 
				count += n; 
				fp = iobuff; 
			} 
			if ((*fp++ = *lp++) == 0) { 
				if (line1 != line2) { 
					nlines++; 
					fp[-1] = '\n'; 
				} else 
					fp--;	/* Don't write the NULL!! */ 
				break; 
			} 
		} 
		line1 = line1->l_next; 
		char1 = 0; 
	} 
	n = fp - iobuff; 
 
	if (write(io, iobuff, n) != n) 
		goto werror; 
	count += n; 
	getDOT();		/* What ever was in linebuf */ 
	return; 
werror: 
	error(WERROR); 
	/* NOTREACHED */ 
} 
 
read_file(file) 
char	*file; 
{ 
	Bufpos	save; 
 
	setcmode(); 
 
	io = open(file, 0); 
	if (io == -1) { 
		curbuf->b_ino = -1; 
		s_mess(IOerr("open", file)); 
		return; 
	} 
	DOTsave(&save); 
	set_ino(curbuf); 
	dofread(file); 
	SetDot(&save); 
	getDOT(); 
	IOclose(); 
} 
 
FileMess(lines, chars) 
long	chars; 
{ 
	add_mess(" %d lines, %D characters", lines, chars); 
} 
 
dofread(file) 
char	*file; 
{ 
	char	end[LBSIZE]; 
	int	xeof = 0; 
 
	lsave(); 
	nlines = 0; 
	count = 0L; 
	f_mess("\"%s\"" , PathRelative(file)); 
	ignore(getline(curline->l_dline, end)); 
	strcpy(genbuf, end); 
	strcpy(end, &end[curchar]); 
	if ((xeof = getfline(linebuf)) == 0) 
		linecopy(genbuf, curchar, linebuf); 
 
	curline->l_dline = putline(genbuf); 
	if (!xeof) do { 
		xeof = getfline(linebuf); 
		nlines++; 
		curline = listput(curbuf, curline); 
		curline->l_dline = putline(linebuf) | DIRTY; 
	} while (!xeof); 
 
	linecopy(linebuf, (curchar = strlen(linebuf)), end); 
	curline->l_dline = putline(linebuf); 
	FileMess(nlines, count); 
} 
 
SaveFile() 
{ 
	if (IsModified(curbuf)) { 
		filemunge(curbuf->b_fname); 
		file_write(curbuf->b_fname, 0); 
		SetUnmodified(curbuf); 
	} else 
		message("No changes need be written"); 
} 
 
static 
fexist_p(filename) 
	char *filename; 
{ 
	struct stat status; 
 
	if (stat(filename, &status)) 
		return(0); 
	else 
		return(1); 
} 
 
#ifndef CHDIR 
 
char * 
PathRelative(fname) 
char	*fname; 
{ 
	return fname; 
} 
 
makefile_exists() 
{ 
	return(1); 
} 
 
#endif CHDIR 
 
#ifdef CHDIR 
char	CurDirectory[256] = {0};		/* Current directory */ 
 
makefile_exists() 
{ 
	char filename[MAXFILLEN]; 
	int i; 
	char *cp; 
 
	i = strlen(CurDirectory); 
	bcopy(CurDirectory, filename, i); 
	cp = filename + i; 
	bcopy("/Makefile", cp, 10); 
	if (fexist_p(filename)) 
		return(1); 
	bcopy("/makefile", cp, 10); 
	if (fexist_p(filename)) 
		return(1); 
	return(0); 
} 
 
char * 
PathRelative(fname) 
char	*fname; 
{ 
	int	n = numcomp(fname, CurDirectory); 
 
	if (n < strlen(CurDirectory) || strcmp(CurDirectory, "/") == 0) 
		n = 0; 
	return fname + n + (n != 0); 
	/* If n == 0 then we print the whole pathname; else we add 1 to move 
	   past the '/' just before the last element of the path */ 
} 
 
Chdir() 
 
{ 
	char	*newdir, 
		dirbuf[MAXFILLEN]; 
 
	newdir = ask_file(getenv("HOME"), FuncName(), dirbuf, DIR_ONLY); 
	if (chdir(dirbuf) == -1) { 
		s_mess("Cannot change into %s", dirbuf); 
		return; 
	} 
	UpdModLine++; 
	strcpy(CurDirectory, dirbuf); 
} 
 
#ifndef JOVE4.2 
char * 
getwd() 
{ 
	Window	*savewp = curwind; 
	char	*dir; 
 
	ignore(UnixToBuf("pwd", 1, 0, 1, "/bin/pwd", "pwd", 0)); 
	ToFirst(); 
	dir = sprint(linebuf); 
	del_wind(curwind); 
	SetWind(savewp); 
	message(""); 
 
	return dir; 
} 
#endif 
 
 
setCWD(d) 
char	*d; 
{ 
	strcpy(CurDirectory, d); 
} 
 
getCWD() 
{ 
#ifdef JOVE4.2 
	char	pathname[MAXFILLEN]; 
	char	*cwd; 
 
	cwd = (char *)getwd(pathname);		/* use system call */ 
	if (cwd) 
		setCWD(cwd); 
#else 
	char	*cwd = getenv("CWD"); 
 
	if (cwd == 0) { 
		cwd = getwd();		/* use "pwd" process */ 
	} 
	setCWD(cwd); 
#endif 
}	 
 
prCWD() 
{ 
	s_mess("%s=> \"%s\"", FuncName(), CurDirectory); 
} 
 
char * 
dbackup(base, offset, c) 
register char	*base, 
		*offset, 
		c; 
{ 
	while (offset > base && *--offset != c) 
		; 
	return offset; 
} 
 
dfollow(file, into) 
char	*file, 
	*into; 
{ 
	char	*dp, 
		*sp; 
 
	if (*file == '/') {		/* Absolute pathname */ 
		strcpy(into, "/"); 
		file++; 
	} else 
		strcpy(into, CurDirectory); 
	dp = into + strlen(into); 
 
	sp = file; 
	do { 
		if (*file == 0) 
			break; 
		if (sp = index(file, '/')) 
			*sp = 0; 
		if (strcmp(file, ".") == 0) 
			;	/* So it will get to the end of the loop */ 
		else if (strcmp(file, "..") == 0) { 
			*(dp = dbackup(into, dp, '/')) = 0; 
			if (dp == into) 
				strcpy(into, "/"), dp = into + 1; 
		} else { 
			if (into[strlen(into) - 1] != '/') 
				strcat(into, "/"); 
			strcat(into, file); 
		} 
		file = sp + 1; 
	} while (sp != 0); 
} 
#endif CHDIR 
 
/* fhsu: taken from ~/tcsh sources posted on Usenet 
 * expand "old" file name with possible tilde usage 
 *		~person/mumble 
 * expands to 
 *		home_directory_of_person/mumble 
 * into string "new". 
 */ 
 
char * 
tilde (new, old) 
char *new, *old; 
{ 
    extern struct passwd *getpwuid (), *getpwnam (); 
 
    register char *o, *p; 
    register struct passwd *pw; 
    static char person[40] = {0}; 
 
    if (old[0] != '~') 
    { 
	strcpy (new, old); 
	return (new); 
    } 
 
    for (p = person, o = &old[1]; *o && *o != '/'; *p++ = *o++); 
    *p = '\0'; 
 
    if (person[0] == '\0')			/* then use current uid */ 
	pw = getpwuid (getuid ()); 
    else 
	pw = getpwnam (person); 
 
    if (pw == NULL) 
	return (NULL); 
 
    strcpy (new, pw -> pw_dir); 
    (void) strcat (new, o); 
    return (new); 
} 
 
 
/* fhsu: add correct handling of ~name and $var for filenames. */ 
PathParse(name, intobuf) 
char	*name, 
	*intobuf; 
{ 
	char	localbuf[MAXFILLEN]; 
	char	*ep; 
 
	localbuf[0] = 0; 
	if (*name == '~') { 
		if (tilde(intobuf, name) != 0) return; 
		strcpy(localbuf, (ep = getenv("HOME")) ? ep : NullStr); 
		name++; 
	} else if (*name == '$') { 
		register int braces = *++name == '{'; 
		register char *value, savech; 
		ep = braces ? name+1 : name;	/* move over left brace */ 
		while (*name && (braces ? *name != '}' : *name != '/')) { 
#ifdef DEBUG 
			printf("%c\n", *name); 
#endif 
			*name++;		/* find end of env var */ 
		} 
		savech = *name;			/* save separator char */ 
		*name = 0; 
		value = (char *) getenv (ep); 
		*name = savech;			/* put separator back */ 
		if (braces && savech) name++;	/* move over right brace */ 
		if (value) { 
			strcpy(intobuf, value); 
			strcat(intobuf, name); 
#ifdef DEBUG 
			printf("ep: %s; value: %s; name: %s; braces: %d; result: %s\n", 
				ep, value, name, braces, intobuf); 
#endif 
			return;			/* all done expanding */ 
		}; 
	} else if (*name == '\\') 
		name++; 
	strcat(localbuf, name); 
#ifdef CHDIR 
	dfollow(localbuf, intobuf); 
#else 
	strcpy(intobuf, localbuf); 
#endif 
} 
 
filemunge(newname) 
char	*newname; 
{ 
	struct stat	stbuf; 
 
	if (newname == 0) 
		return; 
	if (stat(newname, &stbuf)) 
		return; 
	if (stbuf.st_ino != curbuf->b_ino && (stbuf.st_mode & S_IFMT) != S_IFCHR) 
		confirm("\"%s\" already exists; are you sure? ", newname); 
} 
 
WrtReg() 
{ 
	DoWriteReg(0); 
} 
 
AppReg() 
{ 
	DoWriteReg(1); 
} 
 
int	CreatMode	 = DFLT_MODE; 
 
DoWriteReg(app) 
{ 
	char	fnamebuf[MAXFILLEN], 
		*fname; 
	Mark	*mp = CurMark(); 
 
	/* Won't get here if there isn't a Mark */ 
	fname = ask_file((char *) 0, FuncName(), fnamebuf, FIL_ONLY); 
	if (!app) 
		filemunge(fname); 
	open_file(fname, app); 
 
	f_mess("\"%s\"", PathRelative(fname)); 
	if (inorder(mp->m_line, mp->m_char, curline, curchar)) 
		putreg(mp->m_line, mp->m_char, curline, curchar); 
	else 
		putreg(curline, curchar, mp->m_line, mp->m_char); 
	FileMess(nlines, count); 
	IOclose(); 
} 
 
WriteFile() 
{ 
	char	*fname, 
		fnamebuf[MAXFILLEN]; 
 
	fname = ask_file(curbuf->b_fname, FuncName(), fnamebuf, FIL_ONLY); 
 
	filemunge(fname); 
	setfname(curbuf, fname); 
	file_write(curbuf->b_fname, exp_p); 
	curbuf->b_type = FILE; 
	SetUnmodified(curbuf); 
} 
 
open_file(fname, app) 
char	*fname; 
{ 
	io = -1; 
	if (app) { 
		io = open(fname, 1);	/* Writing */ 
		if (io == -1) 
			io = creat(fname, CreatMode); 
		else 
			dolseek(io, 0L, 2); 
	} else  
		io = creat(fname, CreatMode); 
	if (io == -1) 
		complain(IOerr("create", fname)); 
} 
 
file_write(fname, app) 
char	*fname; 
{ 
	if (fname == 0 || *fname == '\0') 
		complain("I need a file name"); 
 
	open_file(fname, app); 
	set_ino(curbuf); 
 
	f_mess("\"%s\"", PathRelative(fname)); 
 
	if (EndWNewline) {	/* Make sure file ends with a newLine */ 
		Bufpos	save; 
 
		DOTsave(&save); 
		ToLast(); 
		if (length(curline))	/* Not a blank Line */ 
			DoTimes(LineInsert, 1);	/* Make it blank */ 
		SetDot(&save); 
	} 
	putreg(curbuf->b_first, 0, curbuf->b_last, length(curbuf->b_last)); 
	FileMess(nlines, count); 
 
	IOclose(); 
} 
 
setcmode() 
{ 
	int	len = curbuf->b_fname ? strlen(curbuf->b_fname) : 0; 
	char	lastc = curbuf->b_fname[len - 1]; 
	char	*ext; 
 
	if (len < 2) { 
		exp_p = 0; 
		return; 
	} 
	if (curbuf->b_fname[len - 2] == '.') { 
		switch(lastc) { 
			case 'c': 
			case 'h':	Cmode(); break; 
 
			case 'p':	PascalMode(); break; 
 
			case 'e':	EuclidMode(); break; 
 
			case 's':	AsmMode(); break; 
 
			default:	FundMode(); break; 
		} 
		exp_p = 0; 
		return; 
	} 
	if (len < 4) { 
		exp_p = 0; 
		return; 
	} 
	ext = &(curbuf->b_fname[len - 4]); 
	if (*ext == '.') { 
		if (strncmp(ext, ".mss", 4) == 0) 
			ScribeMode(); 
		else if (strncmp(ext, ".clu", 4) == 0) 
			CluMode(); 
		else if (strncmp(ext, ".tex", 4) == 0) 
			TexMode(); 
		else if (strncmp(ext, ".epl", 4) == 0 || 
			 strncmp(ext, ".ppe", 4) == 0) 
			EuclidMode(); 
		else if (strncmp(ext, ".txt", 4) == 0 || 
			 strncmp(ext, ".doc", 4) == 0) 
			TextMode(); 
		else	FundMode(); 
	} else 
		FundMode(); 
	exp_p = 0; 
} 
	 
 
ReadFile() 
{ 
	char	*fname, 
		fnamebuf[MAXFILLEN]; 
 
	if (IsModified(curbuf)) 
		confirm("%s modified, shall I read anyway? ", filename(curbuf)); 
 
	fname = ask_file(curbuf->b_fname, FuncName(), fnamebuf, FIL_ONLY); 
	SetUnmodified(curbuf); 
	setfname(curbuf, fname); 
	initlist(curbuf); 
	read_file(curbuf->b_fname); 
} 
 
InsFile() 
{ 
	char	*fname, 
		fnamebuf[MAXFILLEN]; 
 
	fname = ask_file(curbuf->b_fname, FuncName(), fnamebuf, FIL_ONLY); 
	read_file(fname); 
	SetModified(curbuf); 
} 
 
#ifdef XFILE 
DeleteFile() 
{ 
	char	*fname, 
		fnamebuf[MAXFILLEN], 
		**filenames; 
 
	fname = ask_file((char *)0, FuncName(), fnamebuf, FIL_ONLY); 
#ifdef GLOBBER 
	if (needsGlobbing(fname)) { 
		filenames = (char **) glob(fname, 1); 
		if (filenames) { 
			register char **fnames = filenames; 
			while (*fnames != 0) { 
				if (is_directory(*fnames)) { 
					f_mess("%s: Cannot unlink directory", *fnames++); 
					continue; 
				} 
				if (unlink(*fnames)) 
					f_mess("%s: %s", *fnames, sys_errlist[errno]); 
				fnames++; 
			} 
			blkfree(filenames); 
		} else { 
			complain("No matching files."); 
		} 
		return; 
	} 
#endif 
	if (unlink(fname)) 
		complain(sys_errlist[errno]); 
} 
 
RenameFile() 
{ 
	char	*fname, *tname, 
		fnamebuf[MAXFILLEN], 
		tnamebuf[MAXFILLEN]; 
 
	fname = ask_file(curbuf->b_fname, FuncName(), fnamebuf, FIL_ONLY); 
	tname = ask_file((char *)0, 
			 sprint("%s%s to ", FuncName(), fname), 
			 tnamebuf, FIL_ONLY); 
	if (rename(fname, tname)) 
		complain(sys_errlist[errno]); 
} 
 
#ifdef VENIX 
rename(from, to) 
char *from, *to; 
{ 
	int err; 
 
	if (err = link(from, to)) 
		return(err); 
	return(unlink(from)); 
} 
#endif 
#endif 
 
dolseek(fd, offset, whence) 
long	offset; 
{ 
	if (lseek(fd, offset, whence) == -1) 
		complain("lseek failed"); 
} 
 
/* 
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83 
   
   Much of this code was lifted from VI (ex_temp.c).  These functions 
   deal with (put/get)ing lines in/from the tmp file.  */ 
 
#include "jove_temp.h" 
 
int	DOLsave = 0;	/* Do Lsave flag.  If lines aren't being save 
			   when you think they should have been, this 
			   flag is probably not being set, or is being 
			   cleared before lsave() was called. */ 
 
tmpinit() 
{ 
	tfname = mktemp(TMPFILE); 
	tline = 2; 
	iblock1 = oblock = iblock2 = -1; 
	hitin2 = ichng1 = ichng2 = 0; 
	ignore(close(creat(tfname, 0600))); 
	tmpfd = open(tfname, 2); 
	NullLine = putline(""); 
	if (tmpfd == -1) { 
		putstr(sprint("%s?\n", tfname)); 
		finish(0); 
	} 
} 
 
/* Get a line at `tl' in the tmp file into `buf' which should be LBSIZE 
   long. */ 
 
char * 
getline(tl, buf) 
disk_line	tl; 
char	*buf; 
{ 
	register char	*bp, 
			*lp; 
	register int	nl; 
 
	lp = buf; 
	bp = getblock(tl, READ); 
	nl = nleft; 
	tl &= ~OFFMSK; 
	while (*lp++ = *bp++) { 
		if (--nl == 0) { 
			/* += INCRMT moves tl to the next block in 
			   the tmp file. */ 
			bp = getblock(tl += INCRMT, READ); 
			nl = nleft; 
		} 
	} 
 
	return buf; 
} 
 
/* Put `buf' and return the disk address */ 
 
disk_line 
putline(buf) 
char	*buf; 
{ 
	register char	*bp, 
			*lp; 
	register disk_line	nl; 
	disk_line	tl; 
 
	if (*buf == 0 && NullLine >= 0) 
		return NullLine; 
	lp = buf; 
	tl = tline; 
	bp = getblock(tl, WRITE); 
	nl = nleft; 
	tl &= ~OFFMSK; 
	while (*bp = *lp++) { 
		if (*bp++ == '\n') { 
			*--bp = 0; 
			break; 
		} 
		if (--nl == 0) { 
			bp = getblock(tl += INCRMT, WRITE); 
			nl = nleft; 
		} 
	} 
	nl = tline; 
	tline += (((lp - buf) + BNDRY - 1) >> SHFT) & 077776; 
	return (nl | DIRTY);	/* So it will be redisplayed */ 
} 
 
/* Get a block which contains at least part of the line with the address 
   atl.  Returns a pointer to the block and sets the global variable 
   nleft (number of good characters left in the buffer). */ 
 
char * 
getblock(atl, iof) 
disk_line	atl; 
{ 
	extern int	read(), 
			write(); 
	register int	bno, 
			off; 
 
	bno = (atl >> OFFBTS) & BLKMSK; 
	off = (atl << SHFT) & LBTMSK; 
	if (bno >= NMBLKS) 
		error("Tmp file too large.  Get help"); 
	nleft = BUFSIZ - off; 
	if (bno == iblock1) { 
		ichng1 |= iof; 
		return ibuff1 + off; 
	} 
	if (bno == iblock2) { 
		ichng2 |= iof; 
		return ibuff2 + off; 
	} 
	if (bno == oblock) 
		return obuff + off; 
	if (iof == READ) { 
		if (hitin2 == 0) { 
			if (ichng2) 
				blkio(iblock2, ibuff2, write); 
			ichng2 = 0; 
			iblock2 = bno; 
			blkio(bno, ibuff2, read); 
			hitin2 = 1; 
			return (ibuff2 + off); 
		} 
		hitin2 = 0; 
		if (ichng1) 
			blkio(iblock1, ibuff1, write); 
		ichng1 = 0; 
		iblock1 = bno; 
		blkio(bno, ibuff1, read); 
		return (ibuff1 + off); 
	} 
	if (oblock >= 0) 
		blkio(oblock, obuff, write); 
	oblock = bno; 
	return obuff + off; 
} 
 
#ifdef	VMUNIX 
#define	INCORB	64 
char	incorb[INCORB+1][BUFSIZ]; 
#define	pagrnd(a)	((char *)(((int)a)&~(BUFSIZ-1))) 
#endif 
 
blkio(b, buf, iofcn) 
short	b; 
char	*buf; 
int	(*iofcn)(); 
{ 
 
#ifdef VMUNIX 
	if (b < INCORB) { 
		if (iofcn == read) { 
			bcopy(pagrnd(incorb[b+1]), buf, BUFSIZ); 
			return; 
		} 
		bcopy(buf, pagrnd(incorb[b+1]), BUFSIZ); 
		return; 
	} 
#endif 
	ignore(lseek(tmpfd, (long) (unsigned) b * BUFSIZ, 0)); 
	if ((*iofcn)(tmpfd, buf, BUFSIZ) != BUFSIZ) 
		error("IO error"); 
} 
 
#ifdef VMUNIX 
 
/* block copy from from to to, count bytes */ 
 
/* ARGSUSED */ 
 
bcopy(from, to, count) 
#ifdef vax 
	char *from, *to; 
	int count; 
{ 
 
	asm("	movc3	12(ap),*4(ap),*8(ap)"); 
} 
#else 
	register char *from, *to; 
	register int count; 
{ 
	while ((--count) >= 0) 
		*to++ = *from++; 
} 
#endif vax 
#endif VMUNIX 
 
/* 
 * Save the current contents of linebuf, if it has changed. 
 */ 
 
lsave() 
{ 
	char	tmp[LBSIZE]; 
 
	if (curbuf == 0 || !DOLsave)	/* Nothing modified recently */ 
		return; 
 
	if (!strcmp(linebuf, getline(curline->l_dline, tmp))) 
		return;		/* They are the same. */ 
	SavLine(curline, linebuf);	/* Put linebuf on the disk */ 
	DOLsave = 0; 
} 
 
