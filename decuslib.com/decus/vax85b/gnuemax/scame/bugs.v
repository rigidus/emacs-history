head     1.1;A
access   ; 
symbols  ; 
locks    ; strict; 
comment  @# @; 
 
 
1.1 
date     85.02.24.17.38.16;  author bbanerje;  state Exp; 
branches ; 
next     ; 
 
 
desc 
@This is the distribution of the text editor SCAME as received 
over net.sources.  Two minor bugs were fixed prior to placing 
it under RCS. 
@ 
 
 
 
1.1 
log 
@Initial revision 
@ 
text 
@Negative arguments don't work. 
 
Symbolic links are handled badly. 
 
Ownership of files are changed by writefile(). 
 
vfile() can't handle binary text 
 
C-U 0 C-X E  Repeat until error, doesn't work. 
@ 
