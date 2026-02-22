# SPDX-FileCopyrightText: 2026 ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

all:: src/zz/zz.elf
install:: src/zz/zz.elf
	install -DT src/zz/zz.elf $(DESTDIR)$(prefix)/sys/zz.elf

# Bootstrap zz.
src/zz/zz.elf: $(zpy-files) $(srcdir)/src/zz/main.z
	@mkdir -p $(dir $@)
	@echo "ZPY     $@"
	$(Q)$(PYTHON3) -B $(srcdir)/src/zpy/main.py \
		$(srcdir)/src/zz/main.z \
		$(srcdir)/src/zz/main.z
