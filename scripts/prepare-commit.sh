#!/bin/bash

PATH=$PATH:$(dirname $0)

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

set -e

# determine changed files
if [ -d .svn ]; then
	MODIFIED=$(svn status | sed -ne "s/^[MA] *//p")
elif [ -d .git ]; then
	MODIFIED=$(git status | sed -rne "s/^#	(modified|new file): *//p")
else
	echo No working copy
	exit 1
fi

if [ -z "$MODIFIED" ]; then
	echo nothing was modified
	exit 1
fi

# save original changes
if [ -d .svn ]; then
	REV=$(svn info | sed -ne "s/Revision: //p")
	svn diff >r$REV.diff
elif [ -d .git ]; then
	REV=$(git svn info | sed -ne "s/Revision: //p")
	git diff >r$REV.diff
fi

ASTYLEDIFF=astyle.r$REV.diff
>$ASTYLEDIFF

# reformat
for f in $MODIFIED; do
	case "$f" in
	src/core/spatialite/*|src/core/gps/qextserialport/*)
                echo $f skipped
		continue
		;;

        *.cpp|*.c|*.h|*.cxx|*.hxx|*.c++|*.h++|*.cc|*.hh|*.C|*.H)
                ;;

        *)
                continue
                ;;
        esac

        m=$f.r$REV.prepare

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
else
	rm $ASTYLEDIFF
fi
