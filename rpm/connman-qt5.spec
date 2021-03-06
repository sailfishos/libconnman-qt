Name:       connman-qt5
Summary:    Qt bindings for connman
Version:    1.2.35
Release:    1
License:    ASL 2.0
URL:        https://git.sailfishos.org/mer-core/libconnman-qt
Source0:    %{name}-%{version}.tar.bz2
Requires:   connman >= 1.32+git138
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
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   connman-qt5

%description declarative
This package contains the files necessary to develop
applications using libconnman-qt


%package devel
Summary:    Development files for connman-qt
Group:      Development/Libraries
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
rm -rf %{buildroot}
%qmake5_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libconnman-qt5.so.*

%files declarative
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/MeeGo/Connman

%files devel
%defattr(-,root,root,-)
%{_includedir}/connman-qt5
%{_libdir}/pkgconfig/connman-qt5.pc
%{_libdir}/libconnman-qt5.prl
%{_libdir}/libconnman-qt5.so
