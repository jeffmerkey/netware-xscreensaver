%define	name xscreensaver
%define	version 6.13
# override the default configure macro to remove --disable-dependency-tracking
# which causes this version of xscreensaver to fail rpmbuild on Rhel8 Systems

Summary:	X screen saver and locker
Name:		%{name}
Version:	%{version}
Release:	0
#Epoch:		1
License:	BSD
Group:		Amusements/Graphics
URL:		https://www.jwz.org/xscreensaver/
Source0:	https://www.jwz.org/xscreensaver/xscreensaver-%{version}.tar.gz
Vendor:		Jamie Zawinski <jwz@jwz.org>
Packager:	Jeffrey Merkey <jeffmerkey@gmail.com>
Buildroot:	%{_tmppath}/%{name}-root

Patch1:         netwaresmp-xscreensaver-6.13.patch

BuildRequires:	perl
BuildRequires:	pkgconfig
BuildRequires:	desktop-file-utils
BuildRequires:	intltool
BuildRequires:	libX11-devel
BuildRequires:	libXext-devel
BuildRequires:	libXft-devel
BuildRequires:	libXi-devel
BuildRequires:	libXinerama-devel
BuildRequires:	libXrandr-devel
BuildRequires:	libXt-devel
BuildRequires:	libXxf86vm-devel
BuildRequires:	xorg-x11-proto-devel
BuildRequires:	mesa-libGL-devel
BuildRequires:	mesa-libGLU-devel
#BuildRequires:	libgle-devel
BuildRequires:	pam-devel
BuildRequires:	systemd-devel
BuildRequires:	gtk3-devel
BuildRequires:	gdk-pixbuf2-devel
#BuildRequires:	libglade2
BuildRequires:	libxml2-devel
BuildRequires:	gettext-devel
BuildRequires:	libjpeg-turbo-devel

#Requires: SysVinit
Requires: pam
Requires: /etc/pam.d/system-auth
#Requires: htmlview
#Requires: desktop-backgrounds-basic
Requires: xdg-utils
Requires: systemd-libs
Requires: perl
Requires: (attr or xattr)

Provides: xscreensaver

Obsoletes: xscreensaver-base			< %{version}
Obsoletes: xscreensaver-common			< %{version}
Obsoletes: xscreensaver-data			< %{version}
Obsoletes: xscreensaver-data-extra		< %{version}
Obsoletes: xscreensaver-extra			< %{version}
Obsoletes: xscreensaver-extra-base		< %{version}
Obsoletes: xscreensaver-extra-gss		< %{version}
Obsoletes: xscreensaver-extras			< %{version}
Obsoletes: xscreensaver-extras-base		< %{version}
Obsoletes: xscreensaver-extras-gss		< %{version}
Obsoletes: xscreensaver-extrusion		< %{version}
Obsoletes: xscreensaver-gl			< %{version}
Obsoletes: xscreensaver-gl-base			< %{version}
Obsoletes: xscreensaver-gl-extra		< %{version}
Obsoletes: xscreensaver-gl-extra-gss		< %{version}
Obsoletes: xscreensaver-gl-extras		< %{version}
Obsoletes: xscreensaver-gl-extras-gss		< %{version}
Obsoletes: xscreensaver-lang			< %{version}
Obsoletes: xscreensaver-matrix			< %{version}
Obsoletes: xscreensaver-bsod			< %{version}
Obsoletes: xscreensaver-webcollage		< %{version}
Obsoletes: xscreensaver-screensaver-bsod 	< %{version}
Obsoletes: xscreensaver-screensaver-webcollage	< %{version}

%description
A modular screen saver and locker for the X Window System.
More than 250 display modes are included in this package.

%prep
%setup -q
# %patch<#> is deprecated as of RHEL 10, use %patch -P <#> instead.
%patch -P 1 -p1 
autoreconf -v -f
if [ -x %{_datadir}/libtool/config.guess ]; then
  # use system-wide copy
  cp -p %{_datadir}/libtool/config.{sub,guess} .
fi

%build
archdir="`./config.guess`"
mkdir "$archdir" || exit 1
cd "$archdir" || exit 1
export CFLAGS="${CFLAGS:-${RPM_OPT_FLAGS}}"
CONFIG_OPTS="--prefix=/usr"
ln -s ../configure .
%configure $CONFIG_OPTS
rm -f configure
make

%install
archdir="`./config.guess`"
cd "$archdir" || exit 1

rm -rf   ${RPM_BUILD_ROOT}
mkdir -p ${RPM_BUILD_ROOT}/etc/pam.d
mkdir -p ${RPM_BUILD_ROOT}/usr/lib/systemd/user

make DESTDIR=${RPM_BUILD_ROOT} \
     UPDATE_ICON_CACHE=true \
     SUID_FLAGS= \
     install

dd=%{_builddir}/%{name}-%{version}

( cd ${RPM_BUILD_ROOT} || exit 1
  find * -type f -o -type l |
  grep -vF '/.' |
  sed 's@^@/@'  |
  sort |
  sed -e 's@^\(.*/app-defaults/\)@%config \1@' \
      -e 's@^\(.*/pam\.d/\)@%config(missingok) \1@' \
      -e 's@^\(.*/xscreensaver-auth\)$@%attr(4755,root,root) \1@' \
      -e 's@^\(.*/sonar\)$@%attr(4755,root,root) \1@' \
      -e 's@\(.*/man/.*\)@\1\*@' \
) > $dd/allfiles.txt

#cat $dd/allfiles.txt

#%find_lang %{name}
#cat %{name}.lang >> $dd/allfiles.txt

chmod -R a+r,u+w,og-w ${RPM_BUILD_ROOT}

%clean
rm -rf ${RPM_BUILD_ROOT}

%post
killall -HUP xscreensaver 2>&-

# I think that without this, newly-installed icons don't show up in
# desktop menus or in xscreensaver-settings.  Is there a better way?
#
for f in /usr/share/icons/index.theme     \
         /usr/share/icons/*/index.theme   \
         /usr/share/pixmaps/index.theme   \
         /usr/share/pixmaps/*/index.theme \
; do
  if [ -f "$f" ]; then
    f=`dirname "$f"`
    gtk-update-icon-cache --force --quiet "$f"
  fi
done

%preun
killall -q xscreensaver          || true
killall -q xscreensaver-settings || true
killall -q xscreensaver-command  || true
exit 0

%files -f allfiles.txt
%defattr(-, root, root)

%changelog
* Tue Dec 02 2025 jwz
- Cleanup.
* Mon Jul 31 2023 jwz
- Splitting this into multiple packages is a support nightmare, please don't.
* Mon Nov 16 1998 jwz
- Created.
