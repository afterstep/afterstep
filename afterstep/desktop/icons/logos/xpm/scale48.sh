#/bin/sh
if test "x$1" = "x" ; then
  echo "Usage : scale48.sh <source_image> [<destination_image>]";
else
  if test "x$2" = "x" ; then
    out_fname="$1_out";
    echo "Using output filename $out_fname"
  else
    out_fname=$2;
  fi
  ascompose -n -o $out_fname -t xpm -s "<scale width=48 height=48><img src=$1/> </scale>";
fi
