#!/bin/bash
# Generates changelog day by day
echo "CHANGELOG" > ChangeLog
echo ---------------------- >> ChangeLog
git log --format="%cd %cn" --date=short | sort -u -r | while read DATE DUDE ; do
    echo >> ChangeLog
    echo [$DATE: $DUDE] >> ChangeLog
    GIT_PAGER=cat git log --no-merges --format="%w(80,1,3)* %s" --committer="$DUDE" --since="$DATE 00:00:00" --until="$DATE 24:00:00" | grep -v "empty log message">> ChangeLog
done
