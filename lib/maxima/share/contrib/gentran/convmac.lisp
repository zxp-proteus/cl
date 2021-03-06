;*******************************************************************************
;*                                                                             *
;*  copyright (c) 1988 kent state univ.  kent, ohio 44242                      *
;*                                                                             *
;*******************************************************************************

(defvar piport nil)
(defvar poport nil)
(defvar errport nil)

(defmacro foreach (elt kw1 lst kw2 stmt)
;                                                                              ;
; (foreach elt   -->  (progn (mapc    (function (lambda (elt) stmt)) lst) nil) ;
;       in lst                                                                 ;
;       do stmt)                                                               ;
;                                                                              ;
; (foreach elt   -->  (progn (map     (function (lambda (elt) stmt)) lst) nil) ;
;       on lst                                                                 ;
;       do stmt)                                                               ;
;                                                                              ;
; (foreach elt   -->         (mapcar  (function (lambda (elt) stmt)) lst)      ;
;       in lst                                                                 ;
;  collect stmt)                                                               ;
;                                                                              ;
; (foreach elt   -->         (maplist (function (lambda (elt) stmt)) lst)      ;
;       on lst                                                                 ;
;  collect stmt)                                                               ;
;                                                                              ;
; (foreach elt   -->         (mapcan  (function (lambda (elt) stmt)) lst)      ;
;       in lst                                                                 ;
;     conc stmt)                                                               ;
;                                                                              ;
; (foreach elt   -->         (mapcon  (function (lambda (elt) stmt)) lst)      ;
;       on lst                                                                 ;
;     conc stmt)                                                               ;
;                                                                              ;
  (let ((fcn (cdr (assoc kw2 (cdr (assoc kw1 '((in (do . mapc)
						       (collect . mapcar)
						       (conc . mapcan))
						   (on (do . map)
						       (collect . maplist)
						       (conc . mapcon)))))))))
       (cond ((member fcn '(mapc map))
	      `(progn (,fcn (function (lambda (,elt) ,stmt))
			    ,lst)
		      nil))
	     (t
	      `(,fcn (function (lambda (,elt) ,stmt))
		     ,lst)))))


(defmacro aconc (m1 m2)
;                                              ;
; (aconc lst elt)  -->  (nconc lst (list elt)) ;
;                                              ;
  `(nconc ,m1 (list ,m2)))


(defun compress (m)
;                                    ;
; (compress lst)  -->  (implode lst) ;
;                                    ;
  (coerce m 'string))


(defmacro delete1 (e lst)
;                                            ;
; (delete1 elt lst)  -->  (delete elt lst 1) ;
;                                            ;
  `(delete ,e ,lst :count 1))


(defmacro dskin (m)
;                                        ;
; (dskin filename)  -->  (load filename) ;
;                                        ;
  '(load m))


(defun explode2 (m)
;                                     ;
; (explode2 arg)  -->  (explodec arg) ;
;                                     ;
  (coerce (princ-to-string m) 'list))


(defmacro filep (m)
;                                ;
; (filep str)  -->  (probef str) ;
;                                ;
  (streamp m))


(defmacro flag (&rest m)
;                                                   ;
; (flag varlst fname)  -->  (foreach v in varlst do ;
;                             (putprop v t fname))  ;
;                                                   ;
  `(foreach v in ,(cadr m) do
     (putprop v t ,(caddr m))))


(defun flagp (var fname)
;                                         ;
; (flagp var fname)  -->  (get var fname) ;
;                                         ;
  (get var fname))


(defun geq (m)
;                              ;
; (geq n1 n2)  -->  (>= n1 n2) ;
;                              ;
  (>= n1 n2))


(defun idp (m)
;                               ;
; (idp exp)  -->  (symbolp exp) ;
;                               ;
  (symbolp m))


(defun mkfil (m)
;                                     ;
; (mkfil arg)  -->  (stripdollar arg) ;
;                                     ;
  (cons 'stripdollar m))


(defmacro posn ()
;                       ;
; (posn)  -->  (nwritn) ;
;                       ;
  #+clisp '(SYS::LINE-POSITION)
  #+gcl '(si::file-column *standard-output*))

(defmacro prettyprint (m)
;                                                      ;
; (prettyprint exp)  -->  (prog1 ($prpr exp) (terpri)) ;
;                                                      ;
  `(prog1 ($prpr ,m) (terpri)))


(defun put (id proptype propval)
;                                                               ;
; (put id proptype propval)  -->  (putprop id propval proptype) ;
;                                                               ;
  `(putprop ,id ,propval ,proptype))


(defmacro rds (m)
;                                                   ;
; (rds arg)  -->   (prog1 piport (setq piport arg)) ;
;                                                   ;
  `(prog1 piport (setq piport ,m)))


(defun rederr (m)
;                                ;
; (rederr msg)  -->  (error msg) ;
;                                ;
  (cons 'error m))


(defmacro remflag (varlst fname)
;                                                      ;
; (remflag varlst fname)  -->  (foreach v in varlst do ;
;                                (remprop v fname))    ;
;                                                      ;
  `(foreach v in ,varlst do
     (remprop v ,fname)))


(defmacro repeat (stmt exp)
;                                                             ;
; (repeat stmt exp)  -->  (prog ()                            ;
;                               loop                          ;
;                               stmt                          ;
;                               (cond ((not exp) (go loop)))) ;
;                                                             ;
  `(prog ()
	 loop
	 ,stmt
	 (cond ((not ,exp) (go loop)))))


(defmacro spaces (m)
;                                       ;
; (spaces n)  -->  (do ((i n (sub1 i))) ;
;		       ((< i 1))    ;
;		       (princ " "))     ;
;                                       ;
  `(dotimes (i ,m) (princ " ")))


(defmacro while (exp stmt)
;                                                          ;
; (while exp stmt)  -->  (prog ()                          ;
;                              loop                        ;
;                              (cond (exp                  ;
;                                     stmt                 ;
;                                     (go loop))))         ;
;                                                          ;
  `(prog ()
	 loop
	 (cond (,exp
	        ,stmt
	        (go loop)))))


(defmacro wrs (m)
;                                                  ;
; (wrs arg)  -->  (prog1 poport (setq poport arg)) ;
;                                                  ;
  `(prog1 poport (setq poport ,m)))
