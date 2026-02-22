# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Standard library classes and functions oriented around IO.
"""

from zval import defclass, make_direct_slot_definition

### Define basic interface classes.

ClassInputStream = defclass("INPUT-STREAM")
ClassOutputStream = defclass("OUTPUT-STREAM")
ClassByteInputStream = defclass("BYTE-INPUT-STREAM", sup=[ClassInputStream])
ClassByteOutputStream = defclass("BYTE-OUTPUT-STREAM", sup=[ClassOutputStream])

### Define

ClassZPyFile = defclass(
    "%ZPY-FILE",
    make_direct_slot_definition("FD"),
    sup=[ClassByteInputStream, ClassByteOutputStream],
)
