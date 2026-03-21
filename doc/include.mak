# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

components += doc
doc-dir = doc

$(eval $(call compute_component_variables,doc))

# We can't run defcleanable on doc/book, since it's a directory.
$(call defcleanable,doc/book.d)

all:: doc/book
clean::
	@if [[ -d doc/book ]]; then echo "CLEAN   doc/book"; rm -r doc/book; fi
doc-serve:
	$(Q)mdbook serve --dest-dir "$$(realpath doc/book)" --hostname :: $(srcdir)/doc
.PHONY: doc-serve

doc/book: $(srcdir)/doc/book.toml $(srcdir)/doc/SUMMARY.md
	@echo "MDBOOK  doc"
	$(Q)env RUST_LOG=warn mdbook build --dest-dir $(abspath $@) $(srcdir)/doc

doc/book.d: $(srcdir)/doc/SUMMARY.md
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; \
	trap 'rm -f $@.$$$$' EXIT; \
	perl -ne '/- \[.*\]\(\.\/(.*\.md)\)/ && print " $(srcdir)/doc/", $$1' $< >$@.$$$$; \
	printf 'doc/book: %s\n' $$(<$@.$$$$) >$@
-include doc/book.d
