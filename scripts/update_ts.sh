#!/bin/bash
###########################################################################
#    update_ts.sh
#    ---------------------
#    Date                 : November 2014
#    Copyright            : (C) 2014 by Juergen E. Fischer
#    Email                : jef at norbit dot de
###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################

set -e

scriptsdir=$(readlink -f $(dirname $0))

cleanup() {
	if [ -f i18n/backup.tar ]; then
		echo Restoring files...
		tar -xf i18n/backup.tar
	fi

	echo Removing temporary files
	for i in \
		python/python-i18n.{ts,cpp} \
		python/plugins/*/python-i18n.{ts,cpp} \
		src/plugins/grass/grasslabels-i18n.cpp \
		i18n/backup.tar \
		qgis_ts.pro
	do
		[ -f "$i" ] && rm "$i"
	done

	trap "" EXIT
}

PATH=$QTDIR/bin:$PATH

if type qmake-qt5 >/dev/null 2>&1; then
	QMAKE=qmake-qt5
else
	QMAKE=qmake
fi

if ! type pylupdate5 >/dev/null 2>&1; then
	echo "pylupdate5 not found"
	exit 1
fi

if type lupdate-qt5 >/dev/null 2>&1; then
	LUPDATE=lupdate-qt5
else
	LUPDATE=lupdate
fi

trap cleanup EXIT

echo Updating python translations
cd python
pylupdate5 utils.py {console,pyplugin_installer}/*.{py,ui} -ts python-i18n.ts
perl $scriptsdir/ts2cpp.pl python-i18n.ts python-i18n.cpp
rm python-i18n.ts
cd ..
for i in python/plugins/*/CMakeLists.txt; do
	(
	cd ${i%/*}
	pylupdate5 $(find . -name "*.py" -o -name "*.ui") -ts python-i18n.ts
	perl $scriptsdir/ts2cpp.pl python-i18n.ts python-i18n.cpp
	rm python-i18n.ts
	)
done

echo Updating GRASS module translations
perl scripts/qgm2cpp.pl > src/plugins/grass/grasslabels-i18n.cpp

echo Creating qmake project file
$QMAKE -project -o qgis_ts.pro -nopwd $PWD/src $PWD/python $PWD/i18n

echo Updating translations
$LUPDATE -locations absolute -verbose qgis_ts.pro

echo Updating TRANSLATORS File
$scriptsdir/tsstat.pl > doc/TRANSLATORS
