Name:       connman-qt5
Summary:    Qt bindings for connman
Version:    1.3.1
Release:    1
License:    ASL 2.0
URL:        https://github.com/sailfishos/libconnman-qt/
Source0:    %{name}-%{version}.tar.bz2
Requires:   connman >= 1.32+git208
Requires:   libdbusaccess >= 1.0.4
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libdbusaccess) >= 1.0.4

%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}


%description
This is a library for working with connman using Qt


%package declarative
Summary:    Declarative plugin for Qt Quick for connman-qt
Requires:   %{name} = %{version}-%{release}
Requires:   connman-qt5

%description declarative
This package contains the files necessary to develop
applications using libconnman-qt


%package devel
Summary:    Development files for connman-qt
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains the files necessary to develop
applications using libconnman-qt


%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 -r VERSION=%{version}
%qtc_make %{?_smp_mflags}

%install
%qmake5_install
# MeeGo.Connman legacy import
mkdir -p %{buildroot}%{_libdir}/qt5/qml/MeeGo/Connman
ln -sf ../../Connman/libConnmanQtDeclarative.so %{buildroot}%{_libdir}/qt5/qml/MeeGo/Connman/
sed 's/module Connman/module MeeGo.Connman/' < plugin/qmldir > %{buildroot}%{_libdir}/qt5/qml/MeeGo/Connman/qmldir

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libconnman-qt5.so.*

%files declarative
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/Connman
%{_libdir}/qt5/qml/MeeGo/Connman

%files devel
%defattr(-,root,root,-)
%{_includedir}/connman-qt5
%{_libdir}/pkgconfig/connman-qt5.pc
%{_libdir}/libconnman-qt5.prl
%{_libdir}/libconnman-qt5.so
