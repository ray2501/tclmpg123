%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tclmpg123
Summary:       Tcl bindings for libmpg123
Version:       0.8
Release:       2
License:       LGPL v2.1
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
make DESTDIR=%{buildroot} pkglibdir=%{directory}/%{_lib}/tcl/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{directory}/%{_lib}/tcl
