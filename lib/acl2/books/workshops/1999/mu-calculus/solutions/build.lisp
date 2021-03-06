(certify-book "defung")


:ubt! 1

(certify-book "perm")


:ubt! 1

(certify-book "meta")


:ubt! 1

; Certain symbols in the quoted constant below, e.g., len, are no longer
; necessary because after the book was written *acl2-exports* was enlarged.
; We keep the defconst as shown since it is displayed in Chapter 7.

(defconst *export-symbols*
  (union-eq *acl2-exports*
	    (union-eq '(len true-listp defung defequiv defcong i-am-here
			zp defrefinement defabbrev defstub e0-ord-< 
			e0-ordinalp cdr-cons car-cons car-cdr-elim
		        booleanp booleanp-compound-recognizer commutativity-of-+
			complex-rationalp unicity-of-0 associativity-of-+
			binary-append default-car default-cdr cons-equal
			binary-+ acl2-numberp fix nfix alistp assoc-equal
			put-assoc-equal value-of union-eq defpkg
			a b x y z alist key i j l k m list x-equiv y-equiv 
			x1 x2 x3 x4 x5 y1 y2 y3 y4 y5 z1 z2 z3 z4 z5
			*export-symbols*)
		      *common-lisp-symbols-from-main-lisp-package*)))

(defconst *symbols-kept-as-acl2-symbols*
  '(in remove-el perm perm-reflexive remove-el-swap remove-el-in 
    perm-remove car-perm perm-symmetric perm-in perm-transitive
    perm-remove-el-app perm-app-cons))

(defconst *sets-symbols*
  (union-eq *export-symbols* *symbols-kept-as-ACL2-symbols*))

(defpkg "SETS" *sets-symbols*)
(certify-book "sets" 4)


:ubt! 5
(certify-book "fixpoints" 4)


:ubt! 5
(defconst *FAST-SETS-symbols*
  (union-eq 
   *sets-symbols*
   '(sets::=< sets::== sets::depth sets::no-dups)))

(defpkg "FAST-SETS" *fast-sets-symbols*)
(certify-book "fast-sets" 6)


:ubt! 7

(defconst *relations-symbols*
  (union-eq 
   *export-symbols*
   '(sets::in sets::=< sets::== sets::depth  fast-sets::intersect
     fast-sets::add fast-sets::set-union fast-sets::minus)))

(defpkg "RELATIONS"
  *relations-symbols*)

(certify-book "relations" 8)


:ubt! 9

(defconst *MODEL-CHECK-symbols*
  (union-eq 
   *export-symbols*
   (append '(acl2::perm acl2::remove-el)
	   '(sets::in sets::=< sets::== sets::s< sets::=<-perm sets::no-dups)
	   '(fast-sets::intersect fast-sets::add fast-sets::set-union 
	     fast-sets::set-complement fast-sets::minus fast-sets::cardinality
	     fast-sets::make-list-set fast-sets::remove-dups)
	   '(relations::relationp relations::inverse relations::image
				  relations::rel-range-subset))))

(defpkg "MODEL-CHECK" *model-check-symbols*)

(certify-book "models" 10)


:ubt! 11

(certify-book "syntax" 10)


:ubt! 11
(certify-book "semantics" 10)


:ubt! 11
(certify-book "ctl" 10)


