#!/bin/bash


RE=$(echo $(cut -d: -f1 scripts/spelling.dat | sed -e 's/^/\\</; s/$/\\>|/;') | sed -e 's/| /|/g; s/|$//;')
EX="\.(svn-base|tmp|xpm|ts)|context_help|spelling\.dat"

egrep --color=always "$RE" -ir .  | egrep -iv "$EX"
egrep --color=always "$RE" -i resources/context_help/*-en_US
