;  Copyright (C) 2000 Panagiotis Manolios
 
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
 
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
 
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
;  Written by Panagiotis Manolios who can be reached as follows.
 
;  Email: pete@cs.utexas.edu
 
;  Postal Mail:
;  Department of Computer Sciences
;  Taylor Hall 2.124 [C0500]
;  The University of Texas at Austin
;  Austin, TX 78712-1188 USA

(in-package "ACL2")

(include-book "ma128intserial")

(generate-full-system ma-step ma-p maserial-step maserial-p 
		      maserial-to-ma good-maserial maserial-rank)

(in-theory (disable n convert-regs value-of update-valuation ALU-output 
		    b-to-num excp latch1 serial-excp serial-ALU))

(defthm car-cons-isa
  (equal (car (cons x y))
	 x)
  :rule-classes :built-in-clause)

(prove-web ma-step ma-p maserial-step maserial-p maserial-to-ma maserial-rank)

(wrap-it-up ma-step ma-p maserial-step maserial-p 
	    good-maserial maserial-to-ma maserial-rank)
