# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#
############################################################################


#############################################################################
#
# Configuration Options
#
#############################################################################

# Path to mpi home
# type: string 
%{!?path_to_mpihome: %define path_to_mpihome /usr/local/mpi}

# Name for tests home directory
# type: string 
%{!?test_home: %define test_home %{path_to_mpihome}/tests}

#############################################################################
#
# Configuration Logic
#
#############################################################################
%define _prefix %{path_to_mpihome}
%define _sysconfdir %{path_to_mpihome}/etc
%define _libdir %{path_to_mpihome}/lib
%define _includedir %{path_to_mpihome}/include
# disable debuginfo
%define debug_package %{nil}
# disable broken macro - check_files
%define __check_files %{nil}


#############################################################################
#
# Preamble Section
#
#############################################################################

Summary: MPI Benchmarks and tests
Name: %{?_name:%{_name}}%{!?_name:mpitests}
Version: 3.2
Release: 916
License: BSD
Group: Applications
Source: mpitests-%{version}.tar.gz
Packager: %{?_packager:%{_packager}}%{!?_packager:%{_vendor}}
Vendor: %{?_vendorinfo:%{_vendorinfo}}%{!?_vendorinfo:%{_vendor}}
Distribution: %{?_distribution:%{_distribution}}%{!?_distribution:%{_vendor}}
Prefix: %{_prefix}
Provides: mpitests
BuildRoot: %{?buildroot:%{buildroot}}%{!?buildroot:/var/tmp/%{name}-%{version}-%{release}-root}

%description
Set of popular MPI benchmarks:
IMB-3.2
Presta-1.4.0
OSU benchmarks ver 3.1.1

#############################################################################
#
# Prepatory Section
#
#############################################################################
%prep
%setup -q -n mpitests-%{version}

%install
%{__make} MPIHOME=%{path_to_mpihome}

#############################################################################
#
# Install Section
#
#############################################################################
%{__make} MPIHOME=$RPM_BUILD_ROOT/%{path_to_mpihome} install


#############################################################################
#
# Clean Section
#
#############################################################################
%clean
# We should leave build root for IBED installation
# test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT


#############################################################################
#
# Post (Un)Install Section
#
#############################################################################
%post
# Stub

%postun
# Stub

#############################################################################
#
# Files Section
#
#############################################################################


%files 
%defattr(-, root, root)
%{test_home}/IMB-3.2/IMB-MPI1
%{test_home}/presta-1.4.0/com
%{test_home}/presta-1.4.0/glob
%{test_home}/presta-1.4.0/globalop
%{test_home}/osu_benchmarks-3.1.1/osu_bw
%{test_home}/osu_benchmarks-3.1.1/osu_bibw
%{test_home}/osu_benchmarks-3.1.1/osu_latency
%{test_home}/osu_benchmarks-3.1.1/osu_bcast
%{test_home}/osu_benchmarks-3.1.1/osu_alltoall
%{test_home}/osu_benchmarks-3.1.1/osu_mbw_mr
%{test_home}/osu_benchmarks-3.1.1/osu_multi_lat

#############################################################################
#
# Changelog
#
#############################################################################
%changelog
* Sun Nov 15 2009 Pavel Shamis (pasha@mellanox.co.il)
  Intel MPI benchmark IMB-3.1 was updated to IMB-3.2
  OSU MPI benchmark version 3.0 was updated to 3.1.1
* Thu Oct  2 2008 Pavel Shamis (pasha@mellanox.co.il)
  Intel MPI benchmark IMB-3.0 was updated to IMB-3.1
* Thu Nov 22 2006 Pavel Shamis (pasha@mellanox.co.il)
  Intel MPI benchmark IMB-2.3 was updated to IMB-3.0
  OSU benchmarks were updated from 2.0 to 3.0
  mpitest package version updated from 2.0 to 3.0
  removing old code from spec
* Tue Jun 20 2006 Pavel Shamis (pasha@mellanox.co.il)
  Pallas benchmark 2.2.1 was replaced with Intel MPI benchmark IMB-2.3
  Presta 1.2 was updated to 1.4.0
  OSU benchmarks were updated from 1.0 to 2.0
  mpitest package version updated from 1.0 to 2.0
* Wed Apr 01 2006 Pavel Shamis (pasha@mellanox.co.il)
  Spec file for mpitests was created.
