%define	name	AfterStep
%define	fver	2.00.beta4b
%define	version	2.00.beta4b
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
#Source1:	AfterStep-redhat.tar.gz
#Patch0:
Distribution:	The AfterStep TEAM
Packager:	David Mihm <webmaster@afterstep.org>
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
Requires: %{name}-libs

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

%package libs
summary:	libraries required by afterstep 2.0
version:	%{version}
release:	%{release}
copyright:	GPL
group:		User Interface/Desktops

%description libs
  Libraries neeeded by AfterStep 2.0

%package devel
summary:	AftterStep libs include files
version:	%{version}
release:	%{release}
copyright:	GPL
group:		User Interface/Desktops
Requires: %{name}-libs

%description devel
  AftterStep libs include files

%prep
%setup -q -n %{name}-%{fver}
#%patch0 -p1

# RedHat's version of the startmenu
# rm -rf afterstep/start
# tar xzf $RPM_SOURCE_DIR/AfterStep-redhat.tar.gz

CFLAGS=$RPM_OPT_FLAGS \
./configure --prefix=/usr/X11R6 --datadir=/usr/share \
    --disable-staticlibs --enable-sharedlibs \
	 --with-helpcommand="aterm -e man" \
	 --with-desktops=1 --with-imageloader="qiv --root"

%build
make

if [ -x /usr/bin/sgml2html ]; then sgml2html doc/afterstep.sgml; fi

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/usr/X11R6/bin/{sessreg,xpmroot}
for f in libAfter{Base,Conf,Image,Step}; do
   cp -a $f/$f.so* %{buildroot}/usr/X11R6/lib
done

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc ChangeLog NEW README* TEAM UPGRADE doc/languages doc/licences doc/code TODO doc/*.html
/usr/X11R6/bin/*
/usr/X11R6/lib/*
/usr/X11R6/man/*/*
%config /usr/share/afterstep

%files libs
%defattr(-,root,root)
%doc libAfterImage/doc/* libAfterImage/README
/usr/X11R6/lib/*

%files devel
%defattr(-,root,root)
/usr/X11R6/include/libAfterBase/*
/usr/X11R6/include/libAfterImage/*

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
* Sun Dec 14 2003 Andre Costa <acosta@ar.microlink.com.br>
- split into three different RPMs
- AfterStep-libs is now required for AfterStep
- use qiv instead of xv for root image
- removed check for buildroot location on %clean
- removed references to RH startmenu

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

