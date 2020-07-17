Name: nemo-password-manager
Version: 0.1.0
Release: 1
Summary: D-Bus Service for changing and generating passwords

%define dbus_service_name org.nemo.passwordmanager
%define dbus_service_path /org/nemo/passwordmanager

License: GPLv2+
URL: https://git.merproject.org/mer-core/password-manager
Source: %{name}-%{version}.tar.gz

BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(libshadowutils)
BuildRequires: pam-devel
Requires: dbus
Requires: sailfish-setup >= 0.1.3
Requires(post):  dbus
Requires(preun): dbus
Requires(post):  procps
Requires(preun): procps

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
# Ask running daemon to quit
if pgrep -u root -f %{name} >/dev/null; then
    dbus-send --system --dest=%{dbus_service_name} \
        --print-reply %{dbus_service_path} \
        %{dbus_service_name}.quit >/dev/null 2>&1 || true
fi

%preun
# Ask running daemon to quit
if pgrep -u root -f %{name} >/dev/null; then
    dbus-send --system --dest=%{dbus_service_name} \
        --print-reply %{dbus_service_path} \
        %{dbus_service_name}.quit >/dev/null 2>&1 || true
fi


%files
%defattr(-,root,root,-)
%license COPYING
%{_bindir}/%{name}
%{_unitdir}/*.service
%{_datadir}/dbus-1/system-services/%{dbus_service_name}.service
%{_sysconfdir}/dbus-1/system.d/%{dbus_service_name}.conf
