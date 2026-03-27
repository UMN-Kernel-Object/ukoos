;;; SPDX-FileCopyrightText: ukoOS Contributors
;;;
;;; SPDX-License-Identifier: GPL-3.0-or-later

(progn
  (%zpy:dbg (if 1 2 3))
  (if 4 5 6)
  (%zpy:dbg (%zpy:int/+ 40 2)))
