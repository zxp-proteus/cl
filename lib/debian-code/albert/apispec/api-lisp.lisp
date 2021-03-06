;;; -*- Mode: Lisp; Syntax: Common-Lisp; Package: APISPEC -*-

#|

DESC: apispec/api-lisp.lisp - the code for generating lisp APIs
Copyright (c) 1998-2000 - Stig Erik Sandø

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

|#

;; Rewrite this code later to use PPRINT

(in-package :sds-api-apispec)

(defmethod output-declarations ((obj apispec-toplevel) (lang api-lang-lisp) stream some-name)
  (declare (ignore some-name))
  
  (let ((api-name (string-downcase (car (apispec-toplevel.name obj)))))

    (format stream ";;; -*- Mode: Lisp; Syntax: Common-Lisp; Package: sds-api-~a -*-~2%" api-name)
    (format stream ";;; This file is autogenerated by the SDS API Generator.~2%")
	    
    (format stream "(in-package :sds-api-~a)~%" api-name)
    (format stream "(define-sds-module ~a)~2%" api-name)
    
    (let ((constants (apispec-toplevel.constants obj)))
      (dolist (c constants)
	(let ((val (car (apispec-constant.val c))))
	  (format stream "(def-sds-const ~a \"~a\")~%"
		  (car (apispec-constant.name c))
		  (if val val
		    (car (apispec-constant.name c))))))
      (format stream "~2%"))
    
    (let ((classes (apispec-toplevel.classes obj)))
      (dolist (c classes)
	(output-declarations c lang stream api-name)))
    ))

(defmethod output-declarations ((obj apispec-class) (lang api-lang-lisp) stream some-name)
  
  (declare (ignore some-name))

  (let ((elmname (string-downcase(car (apispec-class.elmname obj))))
	(name (string-downcase(car (apispec-class.name obj)))))
  
    (format stream "(def-sds-class ~a~%   (" name) 
    
    (dolist (v (apispec-class.vars obj))
      (format stream "~a " (car (apispec-var.name v))))
    
    (format stream ")~%  ~a)~2%"
	    (if elmname elmname name))))


  
(defmethod output-code ((obj apispec-toplevel) (lang api-lang-lisp) stream some-name)

  ;; hack
  (output-declarations obj lang stream some-name)
  
  (let ((api-name (string-downcase (car (apispec-toplevel.name obj)))))

;;    (format stream ";;; This file is autogenerated by the SDS API Generator.~2%")
	    
;;    (format stream "(in-package :sds-api-~a)~2%" api-name)
    
    (let ((classes (apispec-toplevel.classes obj)))
      (dolist (c classes)
	(output-code c lang stream api-name)))

    (let ((*print-case* :downcase))
      
      ;; constructors
      (pprint `(create-obj-constructors
		,@(mapcar #'(lambda (x)
			      (let ((name (car (apispec-class.name x)))
				    (elmname  (car (apispec-class.elmname x))))
				(list (intern (string-upcase name))
				      (intern (string-upcase (if elmname elmname name))))))
			  (apispec-toplevel.classes obj)))
	      stream))
    
    
    (terpri stream)
    (terpri stream)		    
	      
    ;; factory stuff

    (format stream "(eval-when (:compile-toplevel :load-toplevel :execute)~%")
    (format stream "  (defclass ~a-factory (xml-factory) ()))~%" api-name)
    
    (format stream "(defun make-~a-factory ()  (make-instance '~a-factory :name \"~a factory\"))~%"
	    api-name
	    api-name
	    api-name)
    (format stream "
(defmethod produce-xml-object ((factory ~a-factory) classname)
  (let ((fn (get-constructor classname))
        (retval nil))
    (if fn 
        (setf retval (apply fn '())) 
      (warn \"No good value from ~~a, asked for: ~~a\" (factory.name factory) classname))
    retval
    ))~%"
  api-name
)))


 
(defmethod output-code ((obj apispec-class) (lang api-lang-lisp) stream api-name)
  
  (let ((class-name (string-downcase (car (apispec-class.name obj)))))
  
    (format stream "(defmethod initialize-instance :after ((obj ~a-~a) &key)~%"
	    api-name class-name)

    (let ((attrs (apispec-class.attrs obj)))
      (when attrs
	(format stream "  (add-attributes obj~%")
	(dolist (a attrs)
	  (format stream "     (~:@(~a~) \"~a\" ~a)~%"
		  (car (apispec-attr.type a))
		  (car (apispec-attr.name a))
		  (car (apispec-attr.var  a))))
	(format stream "  )~%")))

    (let ((attrs (apispec-class.subelems obj)))
      (when attrs
	(format stream "  (add-subelements obj~%")
	(dolist (a attrs)
	  (format stream "     (~:@(~a~) \"~a\" ~a)~%"
		  (car (apispec-subelem.type a))
		  (car (apispec-subelem.name a))
		  (car (apispec-subelem.var  a))))
	(format stream "  )~%")))
    
    (format stream ")~2%")))

