#/bin/sh
if test "x$1" = "x" ; then
  echo "Usage : xpm2png.sh <source_image> [<destination_image>] [alpha_blur_radius]";
else
  if test "x$2" = "x" ; then
    out_fname="$1.png";
    echo "Using output filename $out_fname"
  else
    out_fname=$2;
  fi
  if test "x$3" = "x" ; then
    radius=3;
    echo "Using default blur radius $radius"
  else
    radius=$3;
  fi
  ascompose -n -o $out_fname -t png -c 9 -s "<blur horz=$radius vert=$radius channels=\"a\"><img src=$1/> </blur>";
fi
