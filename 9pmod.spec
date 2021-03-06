%define myversion 1.0
%define myrelease 1

%define kernel_release %(echo ${KERNEL_RELEASE})
%define ___kernel_release %(echo ${KERNEL_RELEASE} | tr - _)

# for weak-modules:
%define KREL %{kernel_release}

# no debuginfo
%global debug_package %{nil}


Summary:       9p modules (v9fs) (source)
Name:          9pmod
Version:       %{myversion}
Release:       %{myrelease}
License:     GPL
Group:         System Environment/Kernel
Source:        9pmod.tar.gz
URL:           www.bull.fr
BuildRoot:     %{_tmppath}/%{name}-%{version}-root-%(id -u -n)

%description
Standalone modules for the 9P filesystem (Source)

%define _rpmdrivername 9pmod-ko-%{version}-%{release}

%package ko
Summary:       9p modules (v9fs) (kernel modules)
Version:       %{myversion}
Release:       %{___kernel_release}%{dist}.Bull.%{myrelease}
Group:         System Environment/Kernel


%description ko
Standalone modules for the 9P filesystem

%prep 
%setup -q -n 9pmod

%build 
make

%install
make INSTALL_MOD_PATH=${RPM_BUILD_ROOT} install

%post ko

if [ -e "/boot/System.map-%{KREL}" ]; then
    /sbin/depmod -aeF /boot/System.map-%{KREL} %{KREL} > /dev/null 2>&1 || :
else
    /sbin/depmod -r -ae %{KREL}
fi

modules=( $(find /lib/modules/%{KREL}/extra/fs/9p /lib/modules/%{KREL}/extra/net/9p | grep '\.ko$') )

if [ -x "/sbin/weak-modules" ]; then
    printf '%s\n' "${modules[@]}" | /sbin/weak-modules --add-modules 2>/dev/null || true
fi

%preun ko

# Save modules list for the postun phase
rpm -ql %_rpmdrivername | grep '\.ko$' > /var/run/rpm-kmod-9pmod-modules

%postun ko

if [ -e "/boot/System.map-%{KREL}" ]; then
    /sbin/depmod -aeF /boot/System.map-%{KREL} %{KREL} > /dev/null 2>&1 || :
else
    /sbin/depmod -r -ae %{KREL}
fi

modules=( $(cat /var/run/rpm-kmod-9pmod-modules) )
rm /var/run/rpm-kmod-9pmod-modules

if [ -x "/sbin/weak-modules" ]; then
    printf '%s\n' "${modules[@]}" | /sbin/weak-modules --remove-modules 2>/dev/null || true
fi


%files ko
%defattr (-,root,root)
/lib/modules
%changelog
