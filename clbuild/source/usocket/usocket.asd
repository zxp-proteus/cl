;;;; -*- Mode: Lisp -*-
;;;; $Id: usocket.asd 519 2010-01-13 09:48:05Z ctian $
;;;; $URL: svn://common-lisp.net/project/usocket/svn/usocket/trunk/usocket.asd $

;;;; See the LICENSE file for licensing information.

(in-package #:cl-user)

(defpackage #:usocket-system
    (:use #:cl #:asdf))

(in-package #:usocket-system)

(defsystem usocket
    :name "usocket"
    :author "Erik Enge & Erik Huelsmann"
    :version "0.5.0"
    :licence "MIT"
    :description "Universal socket library for Common Lisp"
    :depends-on (#+sbcl :sb-bsd-sockets)
    :components ((:file "package")
		 (:module "vendor" :depends-on ("package")
		  :components ((:file "split-sequence")
			       #+mcl (:file "kqueue")))
                 (:file "usocket" :depends-on ("vendor"))
                 (:file "condition" :depends-on ("usocket"))
		 (:module "backend" :depends-on ("condition")
		  :components (#+clisp		(:file "clisp")
			       #+cmu		(:file "cmucl")
			       #+scl		(:file "scl")
			       #+(or sbcl ecl)	(:file "sbcl")
			       #+lispworks	(:file "lispworks")
			       #+mcl		(:file "mcl")
			       #+openmcl	(:file "openmcl")
			       #+allegro	(:file "allegro")
			       #+armedbear	(:file "armedbear")))
		 (:file "server" :depends-on ("backend"))))
