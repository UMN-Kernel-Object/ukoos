;;; SPDX-FileCopyrightText: ukoOS Contributors
;;;
;;; SPDX-License-Identifier: GPL-3.0-or-later

(%zpy:sym/set-macro
  'defun
  (%zpy:lambda defun (name args :rest body)
    (progn
      (%zpy:dbg name args body)
      `(%zpy:sym/set-macro
         ',name
         (%zpy:lambda (macro ,name) ,args
           (progn ,@body))))))

(defun foo (x y)
  (%zpy:int/-
    (%zpy:int/* x y)
    (%zpy:int/+ x y)))
(%zpy:dbg foo (foo 5 6))

(%zpy:dbg (lambda (x) x))
(%zpy:dbg (funcall (lambda (x) x) 123))

(progn
  (%zpy:dbg (if 1 2 3))
  (if 4 5 6)
  (%zpy:dbg (%zpy:int/+ 40 2)))
