%bcond_with wayland
%bcond_with x

Name:       cbhm
Summary:    cbhm application
Version:    0.1.235
Release:    1
Group:      Applications
License:    Apache-2.0
URL:        http://www.samsung.com/
Source0:    %{name}-%{version}.tar.gz
Source1:    %{name}.service
Source2:    %{name}.path

BuildRequires:  cmake
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(appcore-common)
##BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(ecore)
##BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(mm-sound)
BuildRequires:  pkgconfig(mm-common)
##BuildRequires:  pkgconfig(xrandr)
##BuildRequires:  pkgconfig(xi)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  edje-tools
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  gettext
BuildRequires:  pkgconfig(dlog)
%{?systemd_requires}

%description
Description: cbhm application

%prep
%setup -q

%build
%if "%{?tizen_profile_name}" == "wearable"
    export TARGET=2.3-wearable
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export TARGET=2.3-mobile
 %else
   %if "%{?tizen_profile_name}" == "tv"
    export TARGET=2.3-mobile
    %endif
 %endif
%endif
export TARGET=2.3-mobile

%define PREFIX /usr/apps/org.tizen.cbhm

rm -rf CMakeFiles CMackCache.txt && cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}  \
%if %{with wayland}
	-DWAYLAND_SUPPORT=On \
%else
	-DWAYLAND_SUPPORT=Off \
%endif
%if %{with x}
	-DX11_SUPPORT=On
%else
	-DX11_SUPPORT=Off
%endif

make %{?jobs:-j%jobs}

%install
%if "%{?tizen_profile_name}" == "wearable"
    export TARGET=2.3-wearable
%else
 %if "%{?tizen_profile_name}" == "mobile"
    export TARGET=2.3-mobile
 %else
   %if "%{?tizen_profile_name}" == "tv"
    export TARGET=2.3-mobile
    %endif
 %endif
%endif
export TARGET=2.3-mobile

%make_install

## systemd
mkdir -p %{buildroot}%{_unitdir_user}/default.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_unitdir_user}/%{name}.service
install -m 0644 %SOURCE2 %{buildroot}%{_unitdir_user}/%{name}.path
ln -s ../%{name}.service %{buildroot}%{_unitdir_user}/default.target.wants/%{name}.service
ln -s ../%{name}.path %{buildroot}%{_unitdir_user}/default.target.wants/%{name}.path

mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/LICENSE %{buildroot}/%{_datadir}/license/%{name}


%post
echo "INFO: System should be restarted or execute: systemctl --user daemon-reload from user session to finish service installation."

chown -R 5000:5000  %{PREFIX}/share

%files
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{PREFIX}/share/edje/cbhmdrawer.edj
%{PREFIX}/share/locale/*
## systemd
%{_unitdir_user}/%{name}.service
%{_unitdir_user}/default.target.wants/%{name}.service
%{_unitdir_user}/%{name}.path
%{_unitdir_user}/default.target.wants/%{name}.path
%{_datadir}/license/%{name}
%manifest %{name}.manifest
