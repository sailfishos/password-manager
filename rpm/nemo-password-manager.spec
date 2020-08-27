Name: nemo-password-manager
Version: 0.1.0
Release: 1
Summary: D-Bus Service for changing and generating passwords

%define dbus_service_name org.nemo.passwordmanager
%define systemd_service_name dbus-org.nemo.passwordmanager.service

License: GPLv2+
URL: https://git.merproject.org/mer-core/password-manager
Source: %{name}-%{version}.tar.gz

BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(libshadowutils)
BuildRequires: pam-devel
BuildRequires: systemd
Requires: dbus
Requires: sailfish-setup >= 0.1.3
Requires: systemd
Requires(post):  dbus
Requires(postun): dbus
%systemd_requires

%description
Password Manager manages user account passwords for developer mode.
It can generate random passwords or set user-supplied passwords.

%prep
%setup -q -n %{name}-%{version}


%build
%qmake5
%make_build

%install
rm -rf %{buildroot}
%qmake5_install

%clean
rm -rf %{buildroot}

%post
%systemd_post %{systemd_service_name}

%preun
%systemd_preun %{systemd_service_name}

%postun
%systemd_postun_with_restart %{systemd_service_name}

%files
%defattr(-,root,root,-)
%license COPYING
%{_bindir}/%{name}
%{_unitdir}/*.service
%{_datadir}/dbus-1/system-services/%{dbus_service_name}.service
%{_sysconfdir}/dbus-1/system.d/%{dbus_service_name}.conf
