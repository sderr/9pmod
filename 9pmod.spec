%define myversion 1.0
%define myrelease 1

%define kernel_release %(echo ${KERNEL_RELEASE})
%define ___kernel_release %(echo ${KERNEL_RELEASE} | tr - _)

Summary:       9p modules (v9fs)
Name:          9pmod
Version:       %{myversion}
Release:       %{___kernel_release}%{dist}.Bull.%{myrelease}
License:     GPL
Group:         System Environment/Kernel
Source:        9pmod.tar.gz
URL:           www.bull.fr
ExclusiveOS:   linux
BuildRoot:     %{_tmppath}/%{name}-%{version}-root-%(id -u -n)

%description
Standalone modules for the 9P filesystem

%prep
%setup -q -n 9pmod

%build 
make

%install
make INSTALL_MOD_PATH=${RPM_BUILD_ROOT} install

%files
%defattr (-,root,root)
/lib/modules
%changelog
