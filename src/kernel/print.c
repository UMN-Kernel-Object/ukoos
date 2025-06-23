#include <physical.h>
#include <print.h>

/**
 * The definition of a formatter and the default formatter.
 */

struct formatter {
  void (*write_byte)(void *ctx, u8 byte);
  void *ctx;
};

// TODO: Instead of hard-coding one way to print (and assuming the address
// is correct for it!), go through a proper driver layer.
static void default_formatter_write_byte(void *ctx, u8 byte) {
  (void)ctx;
  physical_write_u8(paddr_of_bits(0x10000000), byte);
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
  const char *alphabet = "0123456789abcdef";
  char buf_rev[64];
  usize buf_len = 0;
  if (number == 0) {
    buf_rev[0] = '0';
    buf_len = 1;
  } else {
    while (number) {
      buf_rev[buf_len++] = alphabet[number % base];
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

static void format_paddr(struct formatter *fmt, const char *args_start,
                         const char *args_end, va_list *ap) {
  if (args_start == args_end) {
    args_start = "#018x";
    args_end = args_start + strlen(args_start);
  }
  format_number(fmt, args_start, args_end, false,
                (u64)paddr_to_bits(va_arg(*ap, paddr)));
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
    if (!memcmp(type_name_ptr, "u8", 2))
      return format_u8;
    if (!memcmp(type_name_ptr, "va", 2))
      return format_va;
    break;
  case 3:
    if (!memcmp(type_name_ptr, "u16", 3))
      return format_u16;
    if (!memcmp(type_name_ptr, "u32", 3))
      return format_u32;
    if (!memcmp(type_name_ptr, "u64", 3))
      return format_u64;
  case 4:
    if (!memcmp(type_name_ptr, "cstr", 4))
      return format_cstr;
    if (!memcmp(type_name_ptr, "uptr", 4))
      return format_uptr;
    break;
  case 5:
    if (!memcmp(type_name_ptr, "paddr", 5))
      return format_paddr;
    if (!memcmp(type_name_ptr, "uaddr", 5))
      return format_uaddr;
    if (!memcmp(type_name_ptr, "usize", 5))
      return format_usize;
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

void print(const char *format, ...) {
  va_list ap;
  va_start(ap);
  vfprint(&default_formatter, format, ap);
  write_byte(&default_formatter, '\n');
  va_end(ap);
}
