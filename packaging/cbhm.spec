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
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(dlog)
%{?systemd_requires}

%description
Description: cbhm application

%prep
%setup -q

%build
#%if 0%{?tizen_version_major} < 3
#TZ_VER_3=0
#%else
#TZ_VER_3=1
#%endif

# sec_build_binary_debug_enable is not enabled now
#%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
# sec_build_binary_debug_enable is not enabled now
#%endif

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="${CFLAGS} -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="${CXXFLAGS} -DTIZEN_ENGINEER_MODE"
export FFLAGS="${FFLAGS} -DTIZEN_ENGINEER_MODE"
%endif

%define _pkg_dir %{TZ_SYS_RO_APP}/org.tizen.%{name}
%define _pkg_shared_dir %{_pkg_dir}/shared

rm -rf CMakeFiles CMackCache.txt && cmake . -DCMAKE_INSTALL_PREFIX=%{_pkg_dir}  \
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
%make_install

## systemd
mkdir -p %{buildroot}%{_unitdir_user}/default.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_unitdir_user}/%{name}.service
install -m 0644 %SOURCE2 %{buildroot}%{_unitdir_user}/%{name}.path
ln -s ../%{name}.service %{buildroot}%{_unitdir_user}/default.target.wants/%{name}.service
ln -s ../%{name}.path %{buildroot}%{_unitdir_user}/default.target.wants/%{name}.path

%post
echo "INFO: System should be restarted or execute: systemctl --user daemon-reload from user session to finish service installation."


%files
%defattr(-,root,root,-)
%{_pkg_dir}/bin/*
%{_pkg_dir}/res/edje/cbhmdrawer.edj
%{_pkg_dir}/res/locale/*
#%{_pkg_shared_dir}/*
## systemd
%{_unitdir_user}/%{name}.service
%{_unitdir_user}/default.target.wants/%{name}.service
%{_unitdir_user}/%{name}.path
%{_unitdir_user}/default.target.wants/%{name}.path
%manifest %{name}.manifest
%license LICENSE.APLv2

