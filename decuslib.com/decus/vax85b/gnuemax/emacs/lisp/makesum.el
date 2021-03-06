;; Generate key binding summary for Emacs 
;; Copyright (C) 1985 Richard M. Stallman. 
 
;; This file is part of GNU Emacs. 
 
;; GNU Emacs is distributed in the hope that it will be useful, 
;; but without any warranty.  No author or distributor 
;; accepts responsibility to anyone for the consequences of using it 
;; or for whether it serves any particular purpose or works at all, 
;; unless he says so in writing. 
 
;; Everyone is granted permission to copy, modify and redistribute 
;; GNU Emacs, but only under the conditions described in the 
;; document "GNU Emacs copying permission notice".   An exact copy 
;; of the document is supposed to have been given to you along with 
;; GNU Emacs so that you can know how you may redistribute it all. 
;; It should be in a file named COPYING.  Among other things, the 
;; copyright notice and this notice must be preserved on all copies. 
 
 
(defun make-key-summary () (interactive) 
  "Make a summary of current key bindings in the current buffer. 
Previous contents of the buffer are killed first." 
  (let ((cur-mode mode-name) 
	(alphabetize 
;This worked by calling sort; cannot do that until subproc code works 
;	 (yes-or-no-p "Alphabetize by command name? ") 
	 )) 
    (message "Making keyboard summary...") 
    (widen) 
    (if (> (buffer-size) 0) 
	(kill-region (dot-min) (dot-max))) 
    (save-window-excursion 
     (describe-bindings) 
     (insert-buffer "*Help*")) 
    (delete-region (dot) (progn (forward-line 1) (dot))) 
    (save-excursion (replace-string "         " "  ")) 
    (save-excursion (replace-string "@   " "<sp>")) 
    (save-excursion (replace-string "^?   " "<del>")) 
    (let ((odot (dot))) 
      (if (re-search-forward "^Local Bindings:" nil t) 
	  (progn 
	   (forward-char -1) 
	   (insert " for " cur-mode " Mode")))) 
    (goto-char (dot-min)) 
    (insert "Emacs command summary, " (substring (current-time-string) 0 10) 
	    ".\n") 
    ;; Delete "key    binding" and underlining of dashes. 
    (delete-region (dot) (progn (forward-line 2) (dot))) 
    (forward-line 1)			;Skip blank line 
    (while (not (eobp)) 
      (let ((beg (dot))) 
	(or (re-search-forward "^$" nil t) 
	    (goto-char (dot-max))) 
	(double-column beg (dot)) 
	(forward-line 1))) 
    (goto-char (dot-min)))) 
 
(defun double-column (start end) 
  (interactive "r") 
  (let (half cnt 
        line lines nlines 
	(from-end (- (dot-max) end))) 
    (setq nlines (count-lines start end)) 
    (if (<= nlines 1) 
	nil 
      (setq half (/ (1+ nlines) 2)) 
      (goto-char start) 
      (save-excursion 
       (forward-line half) 
       (while (< half nlines) 
	 (setq half (1+ half)) 
	 (setq line (buffer-substring (dot) (save-excursion (end-of-line) (dot)))) 
	 (setq lines (cons line lines)) 
	 (delete-region (dot) (progn (forward-line 1) (dot))))) 
      (setq lines (nreverse lines)) 
      (while lines 
	(end-of-line) 
	(indent-to 41) 
	(insert (car lines)) 
	(forward-line 1) 
	(setq lines (cdr lines)))) 
    (goto-char (- (dot-max) from-end)))) 
