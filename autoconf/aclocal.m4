dnl> test to find the hard-to-find libXpm
dnl> mostly copied from AC_PATH_X & AC_PATH_DIRECT, but explictly set

AC_DEFUN(VT_FIND_LIBXPM,
[
AC_REQUIRE_CPP()

# Initialize some more variables set by options.
# The variables have the same names as the options, with
# dashes changed to underlines.

# If we find XPM, set shell vars xpm_includes and xpm_libraries to the
# paths, otherwise set no_xpm=yes.
# Uses ac_ vars as temps to allow command line to override cache and checks.
AC_MSG_CHECKING(for libXpm)

AC_ARG_WITH(xpm_includes,
  [  --with-xpm-includes=DIR use XPM includes in DIR],
  xpm_includes="$withval", xpm_includes=NO)
AC_ARG_WITH(xpm_library,
  [  --with-xpm-library=DIR  use XPM library in DIR],
  xpm_libraries="$withval", xpm_libraries=NO)

# --without-xpm overrides everything else, but does not touch the cache.
AC_ARG_WITH(xpm,
  [  --with-xpm              use XPM])
if test "$with_xpm" = no; then
  have_xpm=disabled
else
  AC_CACHE_VAL(ac_cv_have_xpm, [
  vt_xpm_include_X11=no
  if test -n "$xpm_includes"; then
    vt_xpm_includes=$xpm_includes
  else
    vt_xpm_includes=NO
  fi
  if test -n "$xpm_libraries"; then
    vt_xpm_libraries=$xpm_libraries
  else
    vt_xpm_libraries=NO
  fi

  VT_XPM_DIRECT

  if test "$vt_xpm_includes" = NO -o "$vt_xpm_libraries" = NO; then
    ac_cv_have_xpm="have_xpm=no"
  else
    ac_cv_have_xpm="have_xpm=yes \
        vt_xpm_includes=$vt_xpm_includes vt_xpm_libraries=$vt_xpm_libraries \
	vt_xpm_include_X11=$vt_xpm_include_X11"
  fi])dnl
  eval "$ac_cv_have_xpm"
fi

if test "$have_xpm" != yes; then
  AC_MSG_RESULT($have_xpm)
  no_xpm=yes
else
  if test "$xpm_includes" != NO; then
    if test "$xpm_includes" != "$vt_xpm_includes"; then
      vt_xpm_include_X11=no
      if test -z "$xpm_includes"; then
	AC_TRY_CPP([#include <X11/xpm.h>],
	vt_xpm_include_X11=yes)
      else
        if test -r "$xpm_includes/X11/xpm.h"; then
	  vt_xpm_include_X11=yes
        fi
      fi
    fi
    vt_xpm_includes=$xpm_includes
  fi
  if test "x$xpm_libraries" != xNO; then
    vt_xpm_libraries=$xpm_libraries
  fi

  dnl# make some output so the user actually sees where his libs were found
  if test -z "$vt_xpm_libraries"; then
    dnl# hmm - the library was found by default without special need to specify the path
    dnl# try to find out what dir the lib was found in
    for i in $LDFLAGS; do
	test_dir=`echo $i | sed s@\^-L@@`
	if test -r $test_dir/libXpm.so -o -r $test_dir/libXpm.a; then
	    xpm_libs_write=$test_dir
	    break
	fi
    done
  else
    xpm_libs_write=$vt_xpm_libraries
  fi
  
  # Update the cache value to reflect the command line values.
  ac_cv_have_xpm="have_xpm=yes \
	vt_xpm_includes=$vt_xpm_includes vt_xpm_libraries=$vt_xpm_libraries \
	vt_xpm_include_X11=$vt_xpm_include_X11"
  eval "$ac_cv_have_xpm"
  AC_MSG_RESULT([-I$vt_xpm_includes, -L$xpm_libs_write])
  if test -n "$vt_xpm_includes"; then
    HAVEXPM="NOXPM"
  else
    HAVEXPM="XPM"
  fi
  if test -n "$vt_xpm_includes"; then
    XPM_CFLAGS="-I$vt_xpm_includes"
  fi
  XPM_LIBS="-lXpm"
  if test -n "$vt_xpm_libraries"; then
    XPM_LIBS="-L$vt_xpm_libraries $XPM_LIBS"
  fi
fi

AC_SUBST(HAVEXPM)
AC_SUBST(XPM_CFLAGS)
AC_SUBST(XPM_LIBS)
])

dnl Internal subroutine of VT_FIND_LIBXPM
dnl Set vt_xpm_include and vt_xpm_libr
# -------------- find xpm.h and Xpm.a/Xpm.so/Xpm.sl
AC_DEFUN(VT_XPM_DIRECT,
[if test "$vt_xpm_includes" = NO; then
  # Guess where to find xpm.h

  # First, try using that file with no special directory specified.
AC_TRY_CPP([#include <X11/xpm.h>],
[# We can compile using X headers with no special include directory.
vt_xpm_includes=
vt_xpm_include_X11=yes],
[# Look for the header file in a standard set of common directories.
  for ac_dir in               \
    /usr/include              \
    /usr/include/X11          \
    /usr/include/X11R6        \
    /usr/include/X11R5        \
    /usr/include/X11R4        \
                              \
    /usr/local/include        \
    /usr/local/include/X11    \
    /usr/local/include/X11R6  \
    /usr/local/include/X11R5  \
    /usr/local/include/X11R4  \
                              \
    /usr/X11/include          \
    /usr/X11R6/include        \
    /usr/X11R5/include        \
    /usr/X11R4/include        \
                              \
    /usr/local/X11/include    \
    /usr/local/X11R6/include  \
    /usr/local/X11R5/include  \
    /usr/local/X11R4/include  \
                              \
    /usr/XFree86/include/X11  \
    /usr/X386/include         \
    /usr/x386/include         \
                              \
    /usr/unsupported/include  \
    /usr/athena/include       \
    /usr/local/x11r5/include  \
    /usr/lpp/Xamples/include  \
                              \
    /usr/openwin/include      \
    /usr/openwin/share/include \
    ; \
  do
    if test -r "$ac_dir/X11/xpm.h"; then
      vt_xpm_includes="$ac_dir"
      vt_xpm_include_X11=yes
      break
    else
      if test -r "$ac_dir/xpm.h"; then
        vt_xpm_includes=$ac_dir
        break
      fi
    fi
  done])
fi

if test "$vt_xpm_libraries" = NO; then
  # Check for the libraries.

  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS="$LIBS"
  LIBS="-lXpm $LIBS -lX11"
AC_TRY_LINK([char XpmReadFileToPixmap();], [XpmReadFileToPixmap()],
[LIBS="$ac_save_LIBS"
# We can link libXpm with no special library path.
vt_xpm_libraries=],
[LIBS="$ac_save_LIBS"
# First see if replacing the include by lib works.
for ac_dir in \
    `echo "$vt_xpm_includes" | sed 's,include/X11,lib,;s,include,lib,'` \
    /usr/X11/lib          \
    /usr/X11R6/lib        \
    /usr/X11R5/lib        \
    /usr/X11R4/lib        \
                          \
    /usr/lib/X11          \
    /usr/lib/X11R6        \
    /usr/lib/X11R5        \
    /usr/lib/X11R4        \
                          \
    /usr/local/X11/lib    \
    /usr/local/X11R6/lib  \
    /usr/local/X11R5/lib  \
    /usr/local/X11R4/lib  \
                          \
    /usr/local/lib/X11    \
    /usr/local/lib/X11R6  \
    /usr/local/lib/X11R5  \
    /usr/local/lib/X11R4  \
                          \
    /usr/X386/lib         \
    /usr/x386/lib         \
    /usr/XFree86/lib/X11  \
                          \
    /usr/lib              \
    /usr/local/lib        \
    /usr/unsupported/lib  \
    /usr/athena/lib       \
    /usr/local/x11r5/lib  \
    /usr/lpp/Xamples/lib  \
                          \
    /usr/openwin/lib      \
    /usr/openwin/share/lib \
    ; \
do
  ac_save_LIBS=$LIBS
  LIBS="-L$ac_dir -lXpm $LIBS -lX11"
  AC_TRY_LINK([char XpmReadFileToPixmap();], [XpmReadFileToPixmap()],
  [vt_xpm_libraries=$ac_dir; break;],
  [LIBS=$ac_save_LIBS])
done])
fi
])

# routines to find the jpeg library, the standard AC_CHECK_LIB() didn't
# work on freebsd (VT_FIND_LIBXPM's source used)
AC_DEFUN(VT_FIND_LIBJPEG,
[
AC_REQUIRE_CPP()

AC_MSG_CHECKING(for libjpeg)

AC_ARG_WITH(jpeg_includes,
  [  --with-jpeg-includes=DIR use JPEG includes in DIR],
  jpeg_includes="$withval", jpeg_includes=NO)
AC_ARG_WITH(jpeg_library,
  [  --with-jpeg-library=DIR  use JPEG library in DIR],
  jpeg_libraries="$withval", jpeg_libraries=NO)

AC_ARG_WITH(jpeg,
  [  --with-jpeg              support JPEG image format [yes]])
if test "$with_jpeg" = no; then
  have_jpeg=disabled
else
  AC_CACHE_VAL(ac_cv_have_jpeg, [
  vt_jpeg_include_X11=no
  if test -n "$jpeg_includes"; then
    vt_jpeg_includes=$jpeg_includes
  else
    vt_jpeg_includes=NO
  fi
  if test -n "$jpeg_libraries"; then
    vt_jpeg_libraries=$jpeg_libraries
  else
    vt_jpeg_libraries=NO
  fi

  VT_JPEG_DIRECT

  if test "$vt_jpeg_includes" = NO -o "$vt_jpeg_libraries" = NO; then
    ac_cv_have_jpeg="have_jpeg=no"
  else
    ac_cv_have_jpeg="have_jpeg=yes \
        vt_jpeg_includes=$vt_jpeg_includes vt_jpeg_libraries=$vt_jpeg_libraries \
	vt_jpeg_include_X11=$vt_jpeg_include_X11"
  fi])dnl
  eval "$ac_cv_have_jpeg"
fi

if test "$have_jpeg" != yes; then
  AC_MSG_RESULT($have_jpeg)
  no_jpeg=yes
else
  if test "$jpeg_includes" != NO; then
    if test "$jpeg_includes" != "$vt_jpeg_includes"; then
      vt_jpeg_include_X11=no
      if test -z "$jpeg_includes"; then
	AC_TRY_CPP([#include <X11/jpeglib.h>],
	vt_jpeg_include_X11=yes)
      else
        if test -r "$jpeg_includes/X11/jpeglib.h"; then
	  vt_jpeg_include_X11=yes
        fi
      fi
    fi
    vt_jpeg_includes=$jpeg_includes
  fi
  if test ! -n "$jpeg_libraries"; then
    vt_jpeg_libraries=$jpeg_libraries
  fi

  dnl# make some output so the user actually sees where his libs were found
  if test -z "$vt_jpeg_libraries"; then
    dnl# hmm - the library was found by default without special need to specify the path
    dnl# try to find out what dir the lib was found in
    for i in $LDFLAGS; do
	test_dir=`echo $i | sed s@\^-L@@`
	if test -r $test_dir/libjpeg.so -o -r $test_dir/libjpeg.a; then
	    jpeg_libs_write=$test_dir
	    break
	fi
    done
  else
    jpeg_libs_write=$vt_jpeg_libraries
  fi
  
  # Update the cache value to reflect the command line values.
  ac_cv_have_jpeg="have_jpeg=yes \
	vt_jpeg_includes=$vt_jpeg_includes vt_jpeg_libraries=$vt_jpeg_libraries \
	vt_jpeg_include_X11=$vt_jpeg_include_X11"
  eval "$ac_cv_have_jpeg"
  AC_MSG_RESULT([-I$vt_jpeg_includes, -L$jpeg_libs_write])
  if test -n "$vt_jpeg_includes"; then
    HAVEJPEG="NOJPEG"
  else
    HAVEJPEG="JPEG"
  fi
  if test -n "$vt_jpeg_includes"; then
    JPEG_CFLAGS="-I$vt_jpeg_includes"
  fi
  JPEG_LIBS="-ljpeg"
  if test -n "$vt_jpeg_libraries"; then
    JPEG_LIBS="-L$vt_jpeg_libraries $JPEG_LIBS"
  fi
fi

AC_SUBST(HAVEJPEG)
AC_SUBST(JPEG_CFLAGS)
AC_SUBST(JPEG_LIBS)
])

AC_DEFUN(VT_JPEG_DIRECT,
[if test "$vt_jpeg_includes" = NO; then
  # Guess where to find jpeglib.h

  # First, try using that file with no special directory specified.
AC_TRY_CPP([#include <jpeglib.h>],
[# We can compile using X headers with no special include directory.
vt_jpeg_includes=
vt_jpeg_include_X11=yes],
[# Look for the header file in a standard set of common directories.
  for ac_dir in               \
    /usr/include              \
    /usr/local/include        \
    /usr/include/X11          \
    /usr/include/X11R6        \
    /usr/include/X11R5        \
    /usr/include/X11R4        \
                              \
    /usr/local/include/X11    \
    /usr/local/include/X11R6  \
    /usr/local/include/X11R5  \
    /usr/local/include/X11R4  \
                              \
    /usr/X11/include          \
    /usr/X11R6/include        \
    /usr/X11R5/include        \
    /usr/X11R4/include        \
                              \
    /usr/local/X11/include    \
    /usr/local/X11R6/include  \
    /usr/local/X11R5/include  \
    /usr/local/X11R4/include  \
                              \
    /usr/XFree86/include/X11  \
    /usr/X386/include         \
    /usr/x386/include         \
                              \
    /usr/unsupported/include  \
    /usr/athena/include       \
    /usr/local/x11r5/include  \
    /usr/lpp/Xamples/include  \
                              \
    /usr/openwin/include      \
    /usr/openwin/share/include \
    ; \
  do
    if test -r "$ac_dir/jpeglib.h"; then
      vt_jpeg_includes="$ac_dir"
      vt_jpeg_include_X11=yes
      break
    else
      if test -r "$ac_dir/jpeglib.h"; then
        vt_jpeg_includes=$ac_dir
        break
      fi
    fi
  done])
fi

if test "$vt_jpeg_libraries" = NO; then
  # Check for the libraries.

  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS="$LIBS"
  LIBS="-ljpeg $LIBS"
AC_TRY_LINK([char jpeg_destroy_compress();], [jpeg_destroy_compress()],
[LIBS="$ac_save_LIBS"
# We can link libjpeg with no special library path.
vt_jpeg_libraries=],
[LIBS="$ac_save_LIBS"
# First see if replacing the include by lib works.
for ac_dir in \
    `echo "$vt_jpeg_includes" | sed 's,include/X11,lib,;s,include,lib,'` \
    /usr/lib              \
    /usr/local/lib        \
			  \
    /usr/X11/lib          \
    /usr/X11R6/lib        \
    /usr/X11R5/lib        \
    /usr/X11R4/lib        \
                          \
    /usr/lib/X11          \
    /usr/lib/X11R6        \
    /usr/lib/X11R5        \
    /usr/lib/X11R4        \
                          \
    /usr/local/X11/lib    \
    /usr/local/X11R6/lib  \
    /usr/local/X11R5/lib  \
    /usr/local/X11R4/lib  \
                          \
    /usr/local/lib/X11    \
    /usr/local/lib/X11R6  \
    /usr/local/lib/X11R5  \
    /usr/local/lib/X11R4  \
                          \
    /usr/X386/lib         \
    /usr/x386/lib         \
    /usr/XFree86/lib/X11  \
                          \
    /usr/unsupported/lib  \
    /usr/athena/lib       \
    /usr/local/x11r5/lib  \
    /usr/lpp/Xamples/lib  \
                          \
    /usr/openwin/lib      \
    /usr/openwin/share/lib \
    ; \
do
  ac_save_LIBS=$LIBS
  LIBS="-L$ac_dir -ljpeg $LIBS"
  AC_TRY_LINK([char jpeg_destroy_compress();], [jpeg_destroy_compress()],
  [vt_jpeg_libraries=$ac_dir; break;],
  [LIBS=$ac_save_LIBS])
done])
fi
])

# routines to find the png library, the standard AC_CHECK_LIB() didn't
# work on freebsd (VT_FIND_LIBXPM's source used)
AC_DEFUN(VT_FIND_LIBPNG,
[
AC_REQUIRE_CPP()

AC_MSG_CHECKING(for libpng)

AC_ARG_WITH(png_includes,
  [  --with-png-includes=DIR use PNG includes in DIR],
  png_includes="$withval", png_includes=NO)
AC_ARG_WITH(png_library,
  [  --with-png-library=DIR  use PNG library in DIR],
  png_libraries="$withval", png_libraries=NO)

AC_ARG_WITH(png,
  [  --with-png              support PNG image format [yes]])
if test "$with_png" = no; then
  have_png=disabled
else
  AC_CACHE_VAL(ac_cv_have_png, [
  vt_png_include_X11=no
  if test -n "$png_includes"; then
    vt_png_includes=$png_includes
  else
    vt_png_includes=NO
  fi
  if test -n "$png_libraries"; then
    vt_png_libraries=$png_libraries
  else
    vt_png_libraries=NO
  fi

  VT_PNG_DIRECT

  if test "$vt_png_includes" = NO -o "$vt_png_libraries" = NO; then
    ac_cv_have_png="have_png=no"
  else
    ac_cv_have_png="have_png=yes \
        vt_png_includes=$vt_png_includes vt_png_libraries=$vt_png_libraries \
	vt_png_include_X11=$vt_png_include_X11"
  fi])dnl
  eval "$ac_cv_have_png"
fi

if test "$have_png" != yes; then
  AC_MSG_RESULT($have_png)
  no_png=yes
else
  if test "$png_includes" != NO; then
    if test "$png_includes" != "$vt_png_includes"; then
      vt_png_include_X11=no
      if test -z "$png_includes"; then
	AC_TRY_CPP([#include <png.h>],
	vt_png_include_X11=yes)
      else
        if test -r "$png_includes/png.h"; then
	  vt_png_include_X11=yes
        fi
      fi
    fi
    vt_png_includes=$png_includes
  fi
  if test "x$png_libraries" != xNO; then
    vt_png_libraries=$png_libraries
  fi

  dnl# make some output so the user actually sees where his libs were found
  if test -z "$vt_png_libraries"; then
    dnl# hmm - the library was found by default without special need to specify the path
    dnl# try to find out what dir the lib was found in
    for i in $LDFLAGS; do
	test_dir=`echo $i | sed s@\^-L@@`
	if test -r $test_dir/libpng.so -o -r $test_dir/libpng.a; then
	    png_libs_write=$test_dir
	    break
	fi
    done
  else
    png_libs_write=$vt_png_libraries
  fi

  # Update the cache value to reflect the command line values.
  ac_cv_have_png="have_png=yes \
	vt_png_includes=$vt_png_includes vt_png_libraries=$vt_png_libraries \
	vt_png_include_X11=$vt_png_include_X11"
  eval "$ac_cv_have_png"
  AC_MSG_RESULT([-I$vt_png_includes, -L$png_libs_write])
  if test -n "$vt_png_includes"; then
    HAVEPNG="NOPNG"
  else
    HAVEPNG="PNG"
  fi
  if test -n "$vt_png_includes"; then
    PNG_CFLAGS="-I$vt_png_includes"
  fi
  PNG_LIBS="-lpng -lz -lm"
  if test -n "$vt_png_libraries"; then
    PNG_LIBS="-L$vt_png_libraries $PNG_LIBS"
  fi
fi

AC_SUBST(HAVEPNG)
AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)
])

AC_DEFUN(VT_PNG_DIRECT,
[if test "$vt_png_includes" = NO; then
  # Guess where to find png.h

  # First, try using that file with no special directory specified.
AC_TRY_CPP([#include <png.h>],
[# We can compile using X headers with no special include directory.
vt_png_includes=
vt_png_include_X11=yes],
[# Look for the header file in a standard set of common directories.
  for ac_dir in               \
    /usr/include              \
    /usr/local/include        \
    /usr/include/X11          \
    /usr/include/X11R6        \
    /usr/include/X11R5        \
    /usr/include/X11R4        \
                              \
    /usr/local/include/X11    \
    /usr/local/include/X11R6  \
    /usr/local/include/X11R5  \
    /usr/local/include/X11R4  \
                              \
    /usr/X11/include          \
    /usr/X11R6/include        \
    /usr/X11R5/include        \
    /usr/X11R4/include        \
                              \
    /usr/local/X11/include    \
    /usr/local/X11R6/include  \
    /usr/local/X11R5/include  \
    /usr/local/X11R4/include  \
                              \
    /usr/XFree86/include/X11  \
    /usr/X386/include         \
    /usr/x386/include         \
                              \
    /usr/unsupported/include  \
    /usr/athena/include       \
    /usr/local/x11r5/include  \
    /usr/lpp/Xamples/include  \
                              \
    /usr/openwin/include      \
    /usr/openwin/share/include \
    ; \
  do
    if test -r "$ac_dir/png.h"; then
      vt_png_includes="$ac_dir"
      vt_png_include_X11=yes
      break
    else
      if test -r "$ac_dir/png.h"; then
        vt_png_includes=$ac_dir
        break
      fi
    fi
  done])
fi

if test "$vt_png_libraries" = NO; then
  # Check for the libraries.

  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS="$LIBS"
  LIBS="-lz -lm -lpng $LIBS"
AC_TRY_LINK([char png_get_sRGB();], [png_get_sRGB()],
[LIBS="$ac_save_LIBS"
# We can link libpng with no special library path.
vt_png_libraries=],
[LIBS="$ac_save_LIBS"
# First see if replacing the include by lib works.
for ac_dir in \
    `echo "$vt_png_includes" | sed 's,include/X11,lib,;s,include,lib,'` \
    /usr/lib              \
    /usr/local/lib        \
			  \
    /usr/X11/lib          \
    /usr/X11R6/lib        \
    /usr/X11R5/lib        \
    /usr/X11R4/lib        \
                          \
    /usr/lib/X11          \
    /usr/lib/X11R6        \
    /usr/lib/X11R5        \
    /usr/lib/X11R4        \
                          \
    /usr/local/X11/lib    \
    /usr/local/X11R6/lib  \
    /usr/local/X11R5/lib  \
    /usr/local/X11R4/lib  \
                          \
    /usr/local/lib/X11    \
    /usr/local/lib/X11R6  \
    /usr/local/lib/X11R5  \
    /usr/local/lib/X11R4  \
                          \
    /usr/X386/lib         \
    /usr/x386/lib         \
    /usr/XFree86/lib/X11  \
                          \
    /usr/unsupported/lib  \
    /usr/athena/lib       \
    /usr/local/x11r5/lib  \
    /usr/lpp/Xamples/lib  \
                          \
    /usr/openwin/lib      \
    /usr/openwin/share/lib \
    ; \
do
  ac_save_LIBS=$LIBS
  LIBS="-L$ac_dir -lz -lm -lpng $LIBS"
  AC_TRY_LINK([char png_get_sRGB();], [png_get_sRGB()],
  [vt_png_libraries=$ac_dir; break;],
  [LIBS=$ac_save_LIBS])
done])
fi
])

