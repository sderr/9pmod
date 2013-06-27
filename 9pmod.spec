%define myversion 1.2.9
%define myrelease 6

%define kernel_release %(echo ${KERNEL_RELEASE})
%define ___kernel_release %(echo ${KERNEL_RELEASE} | tr - _)
%define rpm_kernel_release_noarch %(echo ${KERNEL_RELEASE} | sed 's/\.%{_arch}$//')

Summary:       9p modules (v9fs)
Name:          9pmod
Version:       %{myversion}
Release:       %{myrelease}
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
