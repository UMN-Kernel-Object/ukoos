#!/usr/bin/env bash

# SPDX-FileCopyrightText: ukoOS Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

repo=$(realpath "$(dirname "${BASH_SOURCE[0]}")/..")
cd "$repo"
export repo

# Check that there are no uncommitted changes (they'll get nuked if so).
if [[ -n "$(git status --porcelain)" ]]; then
	printf >&2 'There were uncommitted changes; please stash or delete them.\n'
	exit 1
fi

# Find the commit to restore to.
head=$(git rev-parse HEAD)

# Make a temporary directory for the build.
tmp=$(mktemp -d)
export tmp

# On exit, delete the temporary directory and go back to the saved commit.
cleanup() {
	rm -r "$tmp"
	if [[ -f .git/ORIG_HEAD ]]; then
		git rebase --abort
	else
		git reset --hard "$head"
	fi
}
trap cleanup EXIT

# Assemble the command.
cmd=(
'head=$(git rev-parse HEAD)'
'&& dir="$tmp/$head"'
'&& reuse lint'
'&& mkdir "$dir"'
'&& (cd "$dir" && "$repo/configure" --target qemu-riscv64)'
'&& make -C "$dir" clean'
"make -C \"\$dir\" -j $(nproc) -l $(nproc)"
)
cmd=${cmd[*]}


# Checks that each commit since trunk builds.
git rebase \
	--exec "$cmd" \
	--interactive \
	"${1:-trunk}"
