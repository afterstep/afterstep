%define	name	AfterStep
%define	fver	2.00.beta2
%define	version	2.00.beta2
%define	release	1
%define	serial	1

Summary:	AfterStep Window Manager (NeXTalike)
Name:		%{name}
Version:	%{version}
Release:	%{release}
Serial:		%{serial}
Copyright:	GPL
Group:		User Interface/Desktops
URL:		http://www.afterstep.org
Vendor:		The AfterStep Team (see TEAM in docdir)
Source:		ftp://ftp.afterstep.org/devel/snapshots/%{name}-%{fver}.tar.bz2
Source1:	AfterStep-redhat.tar.gz
#Patch0:

Distribution:	The AfterStep TEAM
Packager:	David Mihm <webmaster@afterstep.org>

BuildRoot:	/tmp/%{name}-%{version}-root

%description
  AfterStep is a Window Manager for X which started by emulating the
  NEXTSTEP look and feel, but which has been significantly altered
  according to the requests of various users. Many adepts will tell you
  that NEXTSTEP is not only the most visually pleasant interface, but
  also one of the most functional and intuitive out there. AfterStep
  aims to incorporate the advantages of the NEXTSTEP interface, and add
  additional useful features.

  The developers of AfterStep have also worked very hard to ensure
  stability and a small program footprint. Without giving up too many
  features, AfterStep still works nicely in environments where memory is
  at a premium.

%prep
%setup -q -n %{name}-%{fver}
#%patch0 -p1

# RedHat's version of the startmenu
rm -rf afterstep/start
tar xzf $RPM_SOURCE_DIR/AfterStep-redhat.tar.gz

CFLAGS=$RPM_OPT_FLAGS \
./configure --prefix=/usr/X11R6 --datadir=/usr/share --disable-staticlibs

%build
make

if [ -x /usr/bin/sgml2html ]; then sgml2html doc/afterstep.sgml; fi

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/usr/X11R6/bin/{sessreg,xpmroot}

%clean
DESTDIR=$RPM_BUILD_ROOT; export DESTDIR
[ -n "`echo $DESTDIR | sed -n 's:^/tmp/[^.].*$:OK:p'`" ] && rm -rf $DESTDIR ||
(echo "\$DESTDIR='$DESTDIR' not in '/tmp'?! Check your .spec..."; exit 1)

%files
%defattr(-,root,root)
%doc ChangeLog NEW README* TEAM UPGRADE doc/languages doc/licences doc/code TODO doc/*.html
/usr/X11R6/bin/*
/usr/X11R6/lib/*
/usr/X11R6/man/*/*
%config /usr/share/afterstep

%pre
for i in /usr /usr/local /usr/X11R6 ; do
	if [ -d $i/share/afterstep_old ]; then
		rm -r $i/share/afterstep_old;
	fi
	# %config /usr/share/afterstep should take care of this.
	#if [ -d $i/share/afterstep ]; then
	#	cp -pr $i/share/afterstep $i/share/afterstep_old;
	#	exit;
	#fi
done

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog
* Mon Dec 6 1999 David Mihm <webmaster@afterstep.org>
  [AfterStep-1.7.149-1]
- Updated to current version

* Wed Jun 9 1999 David Mihm <webmaster@afterstep.org>
  [AfterStep-1.7.111-1]
- Now this spec file is included in the distribution.
- Upgrade to latest snaphost 1.7.111
- Many thanks to Ryan Weaver for this spec file to include!!

* Tue Jun  8 1999 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.7.108-2]
- Made changes to spec to configure and install more like RedHat
  installations.
- Added %config to the /usr/share/afterstep listing to allow rpm to
  backup this dir if needed.

* Tue Jun  8 1999 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.7.108-1]
- Added patches 16-18 to make version 1.7.108

* Fri May 28 1999 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.7.105-1]
- Upgraded to 1.7.90 and added patches 1-15 to make it version 1.7.105.
- Made RPM relocatable.
- Building dynamic libs instead of static.

* Mon Feb  8 1999 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.6.10-1]
- Upgraded to 1.6.10

* Mon Jan  4 1999 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.6.6-3]
- Added a pre-install script to check to see if a previous versions
  share directory exists... If one does, it will copy it to afterstep_old.

* Thu Dec 31 1998 Ryan Weaver <ryanw@infohwy.com>
  [AfterStep-1.6.6-2]
- Configuring with no special settings and installing into
  default dirs as per David Mihm <davemann@ionet.net>

