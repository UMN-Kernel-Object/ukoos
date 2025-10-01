#include <mm/alloc.h>
#include <physical.h>
#include <print.h>

/**
 * The definition of a formatter and the default formatter.
 */

struct formatter {
  void (*write_byte)(void *ctx, u8 byte);
  void *ctx;
};

// TODO: Instead of hard-coding one way to print, go through a proper driver
// layer.
static void default_formatter_write_byte(void *ctx, u8 byte) {
  (void)ctx;

  register uaddr a0 __asm__("a0") = byte;
  register uaddr a1 __asm__("a1");
  register uaddr a6 __asm__("a6") = 0;
  register uaddr a7 __asm__("a7") = 1;

  __asm__ volatile("ecall\n"
                   : "+r"(a0), "=r"(a1)
                   : "r"(a6), "r"(a7)
                   : "memory");
}

static struct formatter default_formatter = {
    .write_byte = default_formatter_write_byte,
    .ctx = nullptr,
};

/**
 * Formatter helpers.
 */

static void write_byte(struct formatter *fmt, u8 byte) {
  fmt->write_byte(fmt->ctx, byte);
}

static void write_str(struct formatter *fmt, const char *str) {
  char ch;
  while ((ch = *str++))
    write_byte(fmt, ch);
}

static void write_stri(struct formatter *fmt, const char *start,
                       const char *end) {
  while (start != end)
    write_byte(fmt, *start++);
}

/**
 * Format functions.
 */

static void vfprint(struct formatter *fmt, const char *format, va_list ap);

typedef void (*format_func)(struct formatter *fmt, const char *args_start,
                            const char *args_end, va_list *ap);

const char *NUMBER_ALPHABET = "0123456789abcdef";

static void format_number(struct formatter *fmt, const char *args_start,
                          const char *args_end, bool sign, u64 number) {
  // Parse arguments.
  u8 pad_byte = ' ';
  u64 base = 10;
  usize pad = 0;
  bool escaped = false, pad_after_prefix = false;
  while (args_start != args_end) {
    char ch;
    switch ((ch = *args_start++)) {
    case '#':
      escaped = true;
      break;
    case '0':
      pad_byte = '0';
      pad_after_prefix = true;
      break;
    case '1' ... '9':
      pad = ch - '0';
      while (ch = *args_start, '0' <= ch && ch <= '9') {
        pad = (pad * 10) + (ch - '0');
        args_start++;
      }
      break;
    case 'b':
      base = 2;
      break;
    case 'o':
      base = 8;
      break;
    case 'x':
      base = 16;
      break;
    default:
      write_str(fmt, "{{invalid arguments for numeric type: ");
      write_stri(fmt, args_start, args_end);
      write_str(fmt, "}}");
      return;
    }
  }

  // Compute the length of the prefix.
  usize prefix_len = 0;
  if (sign) {
    prefix_len++;
    number = -number;
  }
  if (escaped) {
    switch (base) {
    case 2:
    case 8:
    case 16:
      prefix_len += 2;
      break;
    case 10:
      break;
    }
  }

  // Format the number to the buffer, backwards. This needs a special case for
  // zero, which we'd otherwise terminate on.
  char buf_rev[64];
  usize buf_len = 0;
  if (number == 0) {
    buf_rev[0] = '0';
    buf_len = 1;
  } else {
    while (number) {
      buf_rev[buf_len++] = NUMBER_ALPHABET[number % base];
      number /= base;
    }
  }

  // Compute how much padding we need to output.
  usize pad_len = 0;
  if (prefix_len + buf_len < pad) {
    pad_len = pad - (prefix_len + buf_len);
  }

  // Write out padding, if it goes before the prefix.
  if (!pad_after_prefix)
    for (usize i = 0; i < pad_len; i++)
      write_byte(fmt, pad_byte);

  // Write the prefix.
  if (sign)
    write_byte(fmt, '-');
  if (escaped) {
    switch (base) {
    case 2:
      write_str(fmt, "0b");
      break;
    case 8:
      write_str(fmt, "0o");
      break;
    case 10:
      break;
    case 16:
      write_str(fmt, "0x");
      break;
    }
  }

  // Write out padding, if it goes after the prefix.
  if (pad_after_prefix)
    for (usize i = 0; i < pad_len; i++)
      write_byte(fmt, pad_byte);

  // Write out the number.
  for (usize i = 0; i < buf_len; i++)
    write_byte(fmt, buf_rev[buf_len - i - 1]);
}

static void format_bool(struct formatter *fmt, const char *args_start,
                        const char *args_end, va_list *ap) {
  if (args_start != args_end) {
    write_str(fmt, "{{invalid arguments for type bool: ");
    write_stri(fmt, args_start, args_end);
    write_str(fmt, "}}");
    return;
  }

  write_str(fmt, va_arg(*ap, int) ? "true" : "false");
}

static void format_bytes(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  if (args_start != args_end) {
    write_str(fmt, "{{invalid arguments for type bytes: ");
    write_stri(fmt, args_start, args_end);
    write_str(fmt, "}}");
    return;
  }

  const u8 *ptr = va_arg(*ap, const u8 *);
  usize len = va_arg(*ap, usize);
  for (usize i = 0; i < len; i++) {
    if (i)
      write_byte(fmt, ' ');
    write_byte(fmt, NUMBER_ALPHABET[ptr[i] >> 4]);
    write_byte(fmt, NUMBER_ALPHABET[ptr[i] & 15]);
  }
}

static void format_cstr(struct formatter *fmt, const char *args_start,
                        const char *args_end, va_list *ap) {
  if (args_start != args_end) {
    write_str(fmt, "{{invalid arguments for type cstr: ");
    write_stri(fmt, args_start, args_end);
    write_str(fmt, "}}");
    return;
  }

  write_str(fmt, va_arg(*ap, const char *));
}

static void format_i8(struct formatter *fmt, const char *args_start,
                      const char *args_end, va_list *ap) {
  i8 n = (i8)va_arg(*ap, i32);
  format_number(fmt, args_start, args_end, n < 0, (u64)n);
}

static void format_i16(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  i16 n = (i16)va_arg(*ap, i32);
  format_number(fmt, args_start, args_end, n < 0, (u64)n);
}

static void format_i32(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  i32 n = va_arg(*ap, i32);
  format_number(fmt, args_start, args_end, n < 0, (u64)n);
}

static void format_i64(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  i64 n = va_arg(*ap, i64);
  format_number(fmt, args_start, args_end, n < 0, (u64)n);
}

static void format_indent(struct formatter *fmt, const char *args_start,
                          const char *args_end, va_list *ap) {
  if (args_start != args_end) {
    write_str(fmt, "{{invalid arguments for type indent: ");
    write_stri(fmt, args_start, args_end);
    write_str(fmt, "}}");
    return;
  }

  usize i = va_arg(*ap, usize);
  for (usize j = 0; j < i; j++)
    write_byte(fmt, ' ');
}

static void format_isize(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  isize n = va_arg(*ap, isize);
  format_number(fmt, args_start, args_end, n < 0, (u64)n);
}

static void format_paddr(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  if (args_start == args_end) {
    args_start = "#018x";
    args_end = args_start + strlen(args_start);
  }
  format_number(fmt, args_start, args_end, false,
                (u64)bits_of_paddr(va_arg(*ap, paddr)));
}

static void format_u8(struct formatter *fmt, const char *args_start,
                      const char *args_end, va_list *ap) {
  format_number(fmt, args_start, args_end, false, (u64)(u8)va_arg(*ap, u32));
}

static void format_u16(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  format_number(fmt, args_start, args_end, false, (u64)(u16)va_arg(*ap, u32));
}

static void format_u32(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  format_number(fmt, args_start, args_end, false, (u64)va_arg(*ap, u32));
}

static void format_u64(struct formatter *fmt, const char *args_start,
                       const char *args_end, va_list *ap) {
  format_number(fmt, args_start, args_end, false, (u64)va_arg(*ap, u64));
}

static void format_uaddr(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  if (args_start == args_end) {
    args_start = "#018x";
    args_end = args_start + strlen(args_start);
  }
  format_number(fmt, args_start, args_end, false, (u64)va_arg(*ap, uaddr));
}

static void format_uptr(struct formatter *fmt, const char *args_start,
                        const char *args_end, va_list *ap) {
  if (args_start == args_end) {
    args_start = "#018x";
    args_end = args_start + strlen(args_start);
  }
  format_number(fmt, args_start, args_end, false, (u64)va_arg(*ap, uptr));
}

static void format_usize(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  format_number(fmt, args_start, args_end, false, (u64)va_arg(*ap, usize));
}

static void format_va(struct formatter *fmt, const char *args_start,
                      const char *args_end, va_list *ap) {
  if (args_start != args_end) {
    write_str(fmt, "{{invalid arguments for type va: ");
    write_stri(fmt, args_start, args_end);
    write_str(fmt, "}}");
    return;
  }

  const char *sub_format = va_arg(*ap, const char *);
  va_list sub_ap = va_arg(*ap, va_list);
  vfprint(fmt, sub_format, sub_ap);
}

static format_func find_format_func(const char *type_name_ptr,
                                    usize type_name_len) {
  switch (type_name_len) {
  case 2:
    if (!memcmp(type_name_ptr, "i8", 2))
      return format_i8;
    if (!memcmp(type_name_ptr, "u8", 2))
      return format_u8;
    if (!memcmp(type_name_ptr, "va", 2))
      return format_va;
    break;
  case 3:
    if (!memcmp(type_name_ptr, "i16", 3))
      return format_i16;
    if (!memcmp(type_name_ptr, "i32", 3))
      return format_i32;
    if (!memcmp(type_name_ptr, "i64", 3))
      return format_i64;
    if (!memcmp(type_name_ptr, "u16", 3))
      return format_u16;
    if (!memcmp(type_name_ptr, "u32", 3))
      return format_u32;
    if (!memcmp(type_name_ptr, "u64", 3))
      return format_u64;
  case 4:
    if (!memcmp(type_name_ptr, "bool", 4))
      return format_bool;
    if (!memcmp(type_name_ptr, "cstr", 4))
      return format_cstr;
    if (!memcmp(type_name_ptr, "uptr", 4))
      return format_uptr;
    break;
  case 5:
    if (!memcmp(type_name_ptr, "bytes", 5))
      return format_bytes;
    if (!memcmp(type_name_ptr, "isize", 5))
      return format_isize;
    if (!memcmp(type_name_ptr, "paddr", 5))
      return format_paddr;
    if (!memcmp(type_name_ptr, "uaddr", 5))
      return format_uaddr;
    if (!memcmp(type_name_ptr, "usize", 5))
      return format_usize;
    break;
  case 6:
    if (!memcmp(type_name_ptr, "indent", 6))
      return format_indent;
    break;
  }
  return nullptr;
}

/**
 * Formatting specifiers.
 */

static const char *print_specifier(struct formatter *fmt, const char *format,
                                   va_list *ap) {
  const char *type_name_start = format, *type_name_end, *args_start, *args_end;

  // Read the type name.
  char ch;
  for (;;) {
    ch = *format;
    if (ch == '\0') {
      write_str(fmt, "{{specifier parse error}}");
      return format;
    }
    if (ch == ':') {
      type_name_end = format++;
      args_start = format;
      break;
    }
    if (ch == '}') {
      type_name_end = args_start = args_end = format++;
      break;
    }
    format++;
  }

  // Read any arguments.
  if (ch == ':') {
    for (;;) {
      ch = *format;
      if (ch == '\0') {
        write_str(fmt, "{{specifier parse error}}");
        return format;
      }
      if (ch == '}') {
        args_end = format++;
        break;
      }
      format++;
    }
  }

  // Dispatch to the right formatter.
  usize type_name_len = (usize)(type_name_end - type_name_start);
  format_func func = find_format_func(type_name_start, type_name_len);
  if (!func) {
    write_str(fmt, "{{unknown type: ");
    write_stri(fmt, type_name_start, type_name_end);
    write_str(fmt, "}}");
    return format;
  }

  // Call the format function.
  func(fmt, args_start, args_end, ap);
  return format;
}

/**
 * The default formatter.
 */

static void vfprint(struct formatter *fmt, const char *format, va_list ap) {
  char ch;
  while ((ch = *format++)) {
    switch (ch) {
    case '{':
      if (*format == '{') {
        format++;
        write_byte(fmt, '{');
      } else {
        format = print_specifier(fmt, format, &ap);
      }
      break;
    case '}':
      if (*format == '}') {
        format++;
        write_byte(fmt, '}');
      } else {
        write_str(fmt, "{{unexpected close brace}}");
      }
      break;
    default:
      write_byte(fmt, ch);
    }
  }
}

// TODO: We should have a common library of types for e.g. string buffers.
struct format_formatter_buffer {
  u8 *ptr;
  usize cap, len;
};

static void format_formatter_write_byte(void *ctx, u8 byte) {
  struct format_formatter_buffer *buffer = ctx;

  // If we already OOM'd, don't bother writing anything else out.
  if (!buffer->ptr)
    return;

  // If we need to grow the buffer, do so.
  if (buffer->cap == buffer->len) {
    assert(buffer->cap != 0);
    buffer->cap *= 2;
    u8 *new_ptr = alloc(buffer->cap);
    if (!new_ptr) {
      free(buffer->ptr);
      buffer->ptr = nullptr;
      return;
    }
    memcpy(new_ptr, buffer->ptr, buffer->len);
    free(buffer->ptr);
    buffer->ptr = new_ptr;
  }

  // Append the byte.
  buffer->ptr[buffer->len++] = byte;
}

char *format(const char *format, ...) {
  struct format_formatter_buffer buffer = {
      .ptr = alloc(16),
      .cap = 16,
      .len = 0,
  };
  struct formatter format_formatter = {
      .write_byte = format_formatter_write_byte,
      .ctx = &buffer,
  };

  va_list ap;
  va_start(ap);
  vfprint(&format_formatter, format, ap);
  va_end(ap);

  format_formatter_write_byte(&buffer, '\0');
  return (char *)buffer.ptr;
}

void print(const char *format, ...) {
  va_list ap;
  va_start(ap);
  vfprint(&default_formatter, format, ap);
  write_byte(&default_formatter, '\n');
  va_end(ap);
}
