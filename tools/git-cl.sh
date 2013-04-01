#!/bin/bash
# Generates changelog day by day
OUT_FILE=$1/ChangeLog
echo "CHANGELOG" > $OUT_FILE
echo ---------------------- >> $OUT_FILE
git log --relative --format="%cd %cn" --date=short $1 | sort -u -r | while read DATE DUDE ; do
    echo >> $OUT_FILE
    echo [$DATE: $DUDE] >> $OUT_FILE
    GIT_PAGER=cat git log --relative --no-merges --format="%w(80,1,3)* %s" --committer="$DUDE" --since="$DATE 00:00:00" --until="$DATE 24:00:00" $1 | grep -v "empty log message">> $OUT_FILE
done
