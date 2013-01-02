#!/bin/bash

if test "x$1" == "x"; then
	echo "Usage: unexpand.sh <tab_size>";
	exit;
fi

for f in *.c *.h; do
	cp $f $f.old;
	if `unexpand -t $1 --first-only $f.old > $f.new`; then
		mv $f.new $f;
	else
		rm $f.new;
		rm $f.old;
	fi;
done;
