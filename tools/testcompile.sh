#!/bin/bash
###############################################################################
# Test configuring and compiling AfterStep with different options and 
# combinations of options.  This script generally takes a very long time to 
# run with the default option list.
#
# Since this is a developer-only tool, it should be easy to maintain and run 
# on typical development systems.  Thus it assumes a GNU compilation 
# environment and uses bash instead of sh.
#

optlist=( \
	--disable-shaping \
	--disable-windowlist \
	--disable-availability \
	--disable-fixeditems \
	--disable-saveunders \
	--with-png=no \
	--with-jpeg=no \
	--with-xpm=no \
	--disable-script \
	--enable-different-looknfeels \
	--enable-i18n \
	--disable-texture \
	--disable-availability)
logdir="/tmp/testcompile.$$"

# Keep track of time spent and number of tests made, so we can estimate 
# how much longer the remaining tests will take.

startsec=$SECONDS
n=0

# Figure out the number of tests we'll need to do.

end=1
i=0
while test "x$i" != "x${#optlist[*]}"; do
	end=$[$end * 2]
	i=$[$i + 1]
done

# Create the temporary directory.

echo "Results will be stored in $logdir"
mkdir -p $logdir
chmod 0700 $logdir

# testcompile() does the work of testing a set of options.

function testcompile () {
	i=$1
	opt=""
	name=""
	j=$i;k=0
	while test "x$k" != "x${#optlist[*]}"; do
		if expr $j % 2 > /dev/null; then
			opt="$opt ${optlist[$k]}"
			name="1$name"
		else
			name="0$name"
		fi
		j=$[$j / 2]
		k=$[$k + 1]
	done
	echo "Option list: $opt"
	if test -f $logdir/$name; then
		echo "Output file exists, skipping test"
	else
		echo $opt > $logdir/$name 2>&1
		echo -n "Cleaning up..."
		sec=$SECONDS
		tools/makeasclean > /dev/null 2>&1
		echo " done ($[$SECONDS - $sec] sec)"
		echo -n "Configuring..."
		sec=$SECONDS
		./configure --enable-audit --enable-gdb $opt >> $logdir/$name 2>&1
		echo " done ($[$SECONDS - $sec] sec)"
		echo -n "Compiling..."
		sec=$SECONDS
		make >> $logdir/$name 2>&1
		echo " done ($[$SECONDS - $sec] sec)"
	fi
}

function estimate_time () {
	echo "$n ($[100 * $n / ($end - 1)]%) tests completed, $[$end - $n - 1] tests remaining."
	echo -n "Estimated time until done is"
	sec=$[($end - $n) * ($SECONDS - $startsec) / $n]
	if expr $sec / 86400 > /dev/null; then
		echo -n " $[$sec / 86400] day(s)"
		sec=$[$sec - ($sec / 86400) * 86400]
	fi
	if expr $sec / 3600 > /dev/null; then
		echo -n " $[$sec / 3600] hour(s)"
		sec=$[$sec - ($sec / 3600) * 3600]
	fi
	if expr $sec / 60 > /dev/null; then
		echo -n " $[$sec / 60] minutes(s)"
		sec=$[$sec - ($sec / 60) * 60]
	fi
	if ! test "x$sec" = "x0"; then
		echo -n " $sec second(s)"
	fi
	echo "."
}

# Do single-option tests first.

i=1
while test "x$i" != "x$end"; do
	testcompile $i
	i=$[$i * 2]
	n=$[$n + 1]
	estimate_time
done

# Now do the full combination testing.
# Note that we don't do the compile with no options, as AfterStep is often 
# tested with no options anyway.

i=1
while test "x$i" != "x$end"; do
	testcompile $i
	i=$[$i + 1]
	n=$[$n + 1]
	estimate_time
done
