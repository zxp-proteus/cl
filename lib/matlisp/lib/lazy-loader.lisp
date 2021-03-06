;;; -*- Mode: lisp; Syntax: ansi-common-lisp; Base: 10 -*-
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; Copyright (c) 2000 The Regents of the University of California.
;;; All rights reserved. 
;;; 
;;; Permission is hereby granted, without written agreement and without
;;; license or royalty fees, to use, copy, modify, and distribute this
;;; software and its documentation for any purpose, provided that the
;;; above copyright notice and the following two paragraphs appear in all
;;; copies of this software.
;;; 
;;; IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY
;;; FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
;;; ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
;;; THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF
;;; SUCH DAMAGE.
;;;
;;; THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
;;; INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
;;; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE
;;; PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
;;; CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
;;; ENHANCEMENTS, OR MODIFICATIONS.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;;  This file is used to generate 'lazy-loader.lisp'.  Essentially, when
;;;  the 'configure' script is executed the tokens in this file,
;;;  e.g. FLIBS, get substituted with the appropriate machine specific
;;;  parameters and the resulting file is saved in 'lazy-loader.lisp'.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; $Id: lazy-loader.lisp.in,v 1.16 2005/06/24 04:04:08 rtoy Exp $
;;;
;;; $Log: lazy-loader.lisp.in,v $
;;; Revision 1.16  2005/06/24 04:04:08  rtoy
;;; Update for CMUCL with linkage tables.  In this case, we can just load
;;; up the matlisp shared lib.  But not if we're also using ATLAS, which
;;; usually doesn't come as a shared lib.
;;;
;;; Revision 1.15  2005/06/15 13:13:57  rtoy
;;; SBCL doesn't have load-1-foreign anymore; it's called
;;; load-shared-object.
;;;
;;; From Cyrus Harmon.
;;;
;;; Revision 1.14  2004/03/17 03:28:09  simsek
;;; Adding DFFTPACK support for windows.
;;;
;;; Revision 1.13  2004/03/17 03:26:08  simsek
;;; Windows library is now called libmatlisp.dll.
;;;
;;; Revision 1.12  2003/12/07 15:03:44  rtoy
;;; Add support for SBCL.  I did not test if SBCL works, but CMUCL still
;;; works.
;;;
;;; From Robbie Sedgewick on matlisp-users, 2003-11-13.
;;;
;;; Revision 1.11  2001/04/26 13:42:57  rtoy
;;; ATLAS_DIR variable needs to contain the -L flag.  Otherwise, we get a
;;; dangling -L option if we're not using ATLAS.
;;;
;;; Revision 1.10  2001/04/25 17:50:39  rtoy
;;; o Fix the atlas lib stuff
;;; o Reinstate FLIBS because we need that.
;;;
;;; Revision 1.9  2001/03/19 17:07:47  rtoy
;;; Allow the user to specify using ATLAS libraries.
;;;
;;; Revision 1.8  2001/03/08 14:46:18  rtoy
;;; Rename libmatlispshared.so to libmatlisp.so
;;;
;;; Revision 1.7  2001/03/06 17:49:56  rtoy
;;; CMUCL can use the shared library, so make it so.  Also, it seems we
;;; don't need FLIBS so take that out as well.
;;;
;;; Revision 1.6  2000/10/04 15:38:18  simsek
;;; o Added dfftpack to loaded binaries
;;;  o Added unload-blas-&-lapack-libraries for Allegro image
;;;    saving support
;;;
;;; Revision 1.5  2000/10/04 01:22:21  simsek
;;; o Changed package to MATLISP
;;;   This avoids interning symbols in packages other
;;;   than MATLISP
;;;
;;; Revision 1.4  2000/07/11 02:49:55  simsek
;;; *** empty log message ***
;;;
;;; Revision 1.3  2000/07/11 02:08:19  simsek
;;; Added support for Allegro CL
;;;
;;; Revision 1.2  2000/05/05 21:31:00  simsek
;;; o Added the library libdfftpack to the load list
;;;
;;; Revision 1.1  2000/04/13 23:34:29  simsek
;;; o This file is used by lisp to load foreign libraries.
;;; o Initial revision.
;;;
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(in-package "MATLISP")

#|
;; example of an optimized BLAS/LAPACK load on a Solaris platform

(eval-when (load eval compile)
(defun load-blas-&-lapack-libraries ()
  (ext::load-foreign "matlisp:lib;lazy-loader.o"
		:libraries `("-R/home/eclair1/shift-uav/meta-shift/lib"
			     "-L/home/eclair1/shift-uav/meta-shift/lib"
			     "-R/usr/lib"
			     "-L/usr/lib"
			     "-R/usr/sww/lib"
			     "-L/usr/sww/lib"
			     "-R/usr/sww/opt/SUNWspro-5.0/SC5.0/lib"				
			     "-L/usr/sww/opt/SUNWspro-5.0/SC5.0/lib"				
			     "-latlas"
			     "-llapack"
			     "-lblas"
			     "-lM77"
			     "-lF77"
			     "-lsunmath"
			     "-lm"
			     "-lc"))))
|#


(eval-when (load eval compile)
#+(or :cmu :sbcl)
(defun tokenize-ld-args (s)
  (let ((token "")
	(n (length s))
	(tokens nil))

    (dotimes (i n)
       (declare (type fixnum i))
       (let ((c (char s i)))
	 (case c
	  ((#\return #\linefeed 
	    #\space #\tab #\newline)  (if (not (string= token ""))
					  (progn
					    (push token tokens)
					    (setq token ""))))
	  (t (setq token (concatenate 'string token (string c)))))))

    (if (not (string= token ""))
	(push token tokens))

    (nreverse tokens)))

;; For CMUCL, when we use ATLAS we need lazy-loader.o because ATLAS
;; doesn't (normally) come with shared libs.
#+(and :cmu (or nil (not :linkage-table)))
(defun load-blas-&-lapack-libraries ()
  (ext::load-foreign "matlisp:lib;lazy-loader.o"
		:libraries (tokenize-ld-args
			     (concatenate 'string
			         ""
				 " "
				 ""
				 " "
				 "-L"
				 (namestring
				   (translate-logical-pathname "matlisp:lib"))
				 " "
				 "-R"
				 (namestring
				   (translate-logical-pathname "matlisp:lib"))
				 " "
			         "-lmatlisp"
				 " "
				 " -L/usr/lib/gcc/x86_64-linux-gnu/4.3.3 -L/usr/lib/gcc/x86_64-linux-gnu/4.3.3/../../../../lib -L/lib/../lib -L/usr/lib/../lib -L/usr/lib/gcc/x86_64-linux-gnu/4.3.3/../../.. -lgfortranbegin -lgfortran -lm -lgcc_s"))))

;; For CMUCL with linkage tables, we can just load up the matlisp
;; shared lib.
#+(and cmu :linkage-table (not nil))
(defun load-blas-&-lapack-libraries ()
  (ext:load-foreign (translate-logical-pathname "matlisp:lib;libmatlisp.so")))

#+:sbcl
(defun load-blas-&-lapack-libraries ()
  (sb-alien:load-shared-object "matlisp:lib;libmatlisp.so"))



#+:allegro
(defun load-blas-&-lapack-libraries ()
  #+(or :unix :linux) (load "matlisp:lib;libmatlisp.so")
  #+:mswindows (load "matlisp:lib;libmatlisp.dll"))

(load-blas-&-lapack-libraries))

(defun load-blas-&-lapack-binaries ()
  (load-blas-&-lapack-libraries)
  (load (translate-logical-pathname "matlisp:bin;blas") :verbose nil)
  (load (translate-logical-pathname "matlisp:bin;lapack") :verbose nil)
  (load (translate-logical-pathname
	 "matlisp:bin;dfftpack") :verbose nil))

#+(or :cmu :sbcl)
(defun unload-blas-&-lapack-libraries ()
  nil)

#+(and :allegro (not :mswindows))
(defun unload-blas-&-lapack-libraries ()
  (ff:unload-foreign-library "matlisp:lib;libmatlisp.so"))

#+(and :allegro :mswindows)
(defun unload-blas-&-lapack-libraries ()
  (ff:unload-foreign-library "matlisp:lib;libmatlisp.dll"))

