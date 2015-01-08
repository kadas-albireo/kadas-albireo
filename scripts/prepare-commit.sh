#!/bin/bash
###########################################################################
#    prepare-commit.sh
#    ---------------------
#    Date                 : August 2008
#    Copyright            : (C) 2008 by Juergen E. Fischer
#    Email                : jef at norbit dot de
###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################


PATH=$PATH:$(git rev-parse --show-toplevel)/scripts

if ! type -p astyle.sh >/dev/null; then
	echo astyle.sh not found
	exit 1
fi

if ! type -p colordiff >/dev/null; then
	colordiff()
	{
		cat "$@"
	}
fi

if [ "$1" = "-c" ]; then
	echo "Cleaning..."
	find . \( -name "*.prepare" -o -name "*.astyle" -o -name "*.nocopyright" -o -name "astyle.*.diff" -o -name "sha-*.diff" \) -print -delete
fi

set -e

# determine changed files
if [ -d .svn ]; then
	MODIFIED=$(svn status | sed -ne "s/^[MA] *//p")
elif [ -d .git ]; then
	MODIFIED=$(git status --porcelain| sed -ne "s/^ *[MA]  *//p" | sort -u)
else
	echo No working copy
	exit 1
fi

if [ -z "$MODIFIED" ]; then
	echo nothing was modified
	exit 0
fi

# save original changes
if [ -d .svn ]; then
	REV=r$(svn info | sed -ne "s/Revision: //p")
	svn diff >rev-$REV.diff
elif [ -d .git ]; then
	REV=$(git log -n1 --pretty=%H)
	git diff >sha-$REV.diff
fi

ASTYLEDIFF=astyle.$REV.diff
>$ASTYLEDIFF

# reformat
for f in $MODIFIED; do
	case "$f" in
	src/core/gps/qextserialport/*|src/plugins/dxf2shp_converter/dxflib/src/*|src/plugins/globe/osgEarthQt/*|src/plugins/globe/osgEarthUtil/*)
		echo $f skipped
		continue
		;;

	*.cpp|*.c|*.h|*.cxx|*.hxx|*.c++|*.h++|*.cc|*.hh|*.C|*.H)
		if [ -x "$f" ]; then
			chmod a-x "$f"
		fi
		;;

	*.py)
		perl -i.prepare -pe "s/[\t ]+$//;" $f
		if diff -u $f.prepare $f >>$ASTYLEDIFF; then
			# no difference found
			rm $f.prepare
		fi
		continue
		;;

	*)
		continue
		;;
	esac

	m=$f.$REV.prepare

	cp $f $m
	astyle.sh $f
	if diff -u $m $f >>$ASTYLEDIFF; then
		# no difference found
		rm $m
	fi
done

if [ -s "$ASTYLEDIFF" ]; then
	if tty -s; then
		# review astyle changes
		colordiff <$ASTYLEDIFF | less -r
	else
		echo "Files changed (see $ASTYLEDIFF)"
	fi
	exit 1
else
	rm $ASTYLEDIFF
fi

exit 0

# vim: set ts=8 noexpandtab :
