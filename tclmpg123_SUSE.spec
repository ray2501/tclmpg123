%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclmpg123
Summary:       Tcl bindings for libmpg123
Version:       1.2
Release:       0
License:       LGPL-2.1-or-later
Group:         Development/Libraries/Tcl
Source:        %{name}-%{version}.tar.gz
URL:           https://github.com/ray2501/tclmpg123
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.4
BuildRequires: mpg123-devel
Requires:      tcl >= 8.4
Requires:      libmpg123-0
BuildRoot:     %{buildroot}

%description
Tcl bindings for libmpg123.

%prep
%setup -q -n %{name}-%{version}

%build
./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{tcl_archdir}
