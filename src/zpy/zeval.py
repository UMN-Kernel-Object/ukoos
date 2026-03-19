# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later


from zval import ZVal


def apply(func: ZVal, *args: ZVal) -> tuple[ZVal, ...]:
    raise Exception(f"apply {func} {args}")
