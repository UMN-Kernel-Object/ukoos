# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""
The S-expression parser.
"""

from dataclasses import dataclass
from zval import NIL, ZCons, ZMod, ZStr, ZVal

CURRENT_MOD = ZMod.find("Z")


def is_constituent(rune: str) -> bool:
    return (
        ("0" <= rune <= "9")
        or ("A" <= rune <= "Z")
        or ("a" <= rune <= "z")
        or (rune in "!$%&*+-./:<=>?@[]^_{}")
    )


def is_space(rune: str) -> bool:
    assert rune < "\x80", "TODO: Unicode"
    return rune in "\t\n\r "


@dataclass
class FoundDotException(Exception):
    src: ZStr


@dataclass
class FoundRightParenException(Exception):
    src: ZStr


def read_one_from_string(src: ZStr) -> tuple[ZVal, ZStr | None]:
    while True:
        if len(src) == 0:
            return NIL, None

        rune, rune_len = src.decode_one_utf8()
        match chr(rune):
            case rune if is_space(rune):
                src = src[rune_len:]
                continue
            case rune if is_constituent(rune):
                return read_token_from_string(src)
            case "\\" | "|":
                return read_token_from_string(src)
            case '"':
                raise Exception("read_one_from_string: TODO")
            case "#":
                raise Exception("read_one_from_string: TODO")
            case "'":
                raise Exception("read_one_from_string: TODO")
            case "(":
                return read_list_from_string(src[rune_len:])
            case ")":
                raise FoundRightParenException(src[rune_len:])
            case ",":
                raise Exception("read_one_from_string: TODO")
            case ";":
                while len(src) != 0:
                    rune, rune_len = src.decode_one_utf8()
                    src = src[rune_len:]
                    if chr(rune) == "\n":
                        break
                continue
            case "`":
                raise Exception("read_one_from_string: TODO")
            case rune:
                raise Exception(f"read_one_from_string: unexpected character: {rune!r}")


def read_list_from_string(src: ZStr) -> tuple[ZVal, ZStr | None]:
    box = ZCons(NIL, NIL)
    dst = box
    while True:
        try:
            form, new_src = read_one_from_string(src)
            if new_src is None:
                raise Exception("read_list_from_string: unexpected EOF")
            src = new_src

            dst.cdr = ZCons(form, NIL)
            dst = dst.cdr
        except FoundDotException as exc:
            if dst is box:
                raise Exception("read_list_from_string: unexpected dot")

            src = exc.src

            form, new_src = read_one_from_string(src)
            if new_src is None:
                raise Exception("read_list_from_string: unexpected EOF")
            src = new_src

            dst.cdr = form
            try:
                _, new_src = read_one_from_string(src)
                if new_src is None:
                    raise Exception("read_list_from_string: unexpected EOF")
                else:
                    raise Exception("read_list_from_string: unexpected value")
            except FoundRightParenException as exc:
                return box.cdr, exc.src
        except FoundRightParenException as exc:
            dst.cdr = NIL
            return box.cdr, exc.src


def read_token_from_string(src: ZStr) -> tuple[ZVal, ZStr | None]:
    colon_index = None
    has_double_colon = False
    has_escape = False
    token = ""

    # Eat runes until we get a complete token.
    while True:
        if len(src) == 0:
            break

        rune, rune_len = src.decode_one_utf8()
        s = chr(rune)
        if s == ":":
            if colon_index is None:
                colon_index = len(token.encode("utf-8"))
            elif colon_index == len(token.encode("utf-8")) and not has_double_colon:
                has_double_colon = True
            else:
                raise Exception("read_token_from_string: too many colons")
        elif is_constituent(s):
            token += s.upper()
        elif s == "\\":
            raise Exception("read_token_from_string: TODO")
        elif s == "|":
            raise Exception("read_token_from_string: TODO")
        else:
            break
        src = src[rune_len:]

    # If it's colon-separated, it can't be a number. (However, this means we
    # also skipped the colons in the token, so e.g. 1:2 could parse as an
    # integer.)
    if colon_index is not None:
        token = ZStr(token)
        mod = ZMod.find(token[:colon_index])
        sym, status = mod.intern(token[colon_index:])
        if status != "EXTERNAL" and not has_double_colon:
            raise Exception(
                f"read_token_from_string: {sym} is not exported by {mod.name}"
            )
        return sym, src

    # If we didn't have escape characters, we can try to parse this as a few
    # other things.
    if not has_escape:
        if token == ".":
            raise FoundDotException(src)
        try:
            return int(token), src
        except ValueError:
            pass
        try:
            return float(token), src
        except ValueError:
            pass

    # Otherwise, find it in the current module.
    sym, _ = CURRENT_MOD.intern(ZStr(token))
    return sym, src
