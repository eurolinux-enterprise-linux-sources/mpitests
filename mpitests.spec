Summary: MPI Benchmarks and tests
Name: mpitests
Version: 3.2
Release: 5%{?dist}
License: BSD
Group: Applications/Engineering
# We get the mpitests.tar.gz file from an OFED release.
# Unfortunately, they're not good about changing the name
# of the tarball when they change the contents.
URL: http://www.openfabrics.org
Source: mpitests-%{version}.tar.gz
Patch0: mpitests-2.0-make.patch
Provides: mpitests
# mvapich2 only exists on these three arches
ExclusiveArch: i686 i386 x86_64 ia64

%description
Set of popular MPI benchmarks:
IMB-3.2
Presta-1.4.0
OSU benchmarks ver 3.1.1

%package openmpi
Summary: MPI tests package compiled against openmpi
Group: Applications
BuildRequires: openmpi >= 1.4, openmpi-devel
%description openmpi
MPI test suite compiled against the openmpi package

%package mvapich
Summary: MPI tests package compiled against mvapich
Group: Applications
BuildRequires: mvapich-devel >= 1.2.0
%description mvapich
MPI test suite compiled against the mvapich package

%package mvapich2
Summary: MPI tests package compiled against mvapich2
Group: Applications
BuildRequires: mvapich2-devel >= 1.4
BuildRequires: librdmacm-devel, libibumad-devel
%description mvapich2
MPI test suite compiled against the mvapich2 package

%ifarch x86_64
%package mvapich-psm
Summary: MPI tests package compiled against mvapich using InfiniPath
Group: Applications
BuildRequires: mvapich-psm-devel >= 1.2.0
BuildRequires: infinipath-psm-devel
%description mvapich-psm
MPI test suite compiled against the mvapich package using InfiniPath

%package mvapich2-psm
Summary: MPI tests package compiled against mvapich2 using InfiniPath
Group: Applications
BuildRequires: mvapich2-psm-devel >= 1.4
BuildRequires: librdmacm-devel, libibumad-devel
BuildRequires: infinipath-psm-devel
%description mvapich2-psm
MPI test suite compiled against the mvapich2 package using InfiniPath
%endif

%prep
%setup -q -a 0
# secretly patch the code one layer down, not at the top level
%patch0 -p0 -b .make

%build
RPM_OPT_FLAGS=`echo $RPM_OPT_FLAGS | sed -e 's/-Wp,-D_FORTIFY_SOURCE=.//' | sed -e 's/-fstack-protector//'`
# We don't do a non-mpi version of this package, just straight to the mpi builds
export CC=mpicc
export CXX=mpicxx
export FC=mpif90
export F77=mpif77
export CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"
do_build() { 
  cp -al %{name}-%{version} $MPI_COMPILER
  cd $MPI_COMPILER
  make $*
  cd ..
}


# do N builds, one for each mpi stack
%{_openmpi_load}
do_build all
%{_openmpi_unload}

%{_mvapich_load}
do_build osu-mpi1 presta
%{_mvapich_unload}

%{_mvapich2_load}
do_build all
%{_mvapich2_unload}
%ifarch x86_64

%{_mvapich_psm_load}
do_build osu-mpi1 presta
%{_mvapich_psm_unload}

%{_mvapich2_psm_load}
do_build all
%{_mvapich2_psm_unload}
%endif
%install
rm -rf %{buildroot}
# do N installs, one for each mpi stack
%{_openmpi_load}
mkdir -p %{buildroot}$MPI_BIN
make -C $MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_openmpi_unload}

%{_mvapich_load}
mkdir -p %{buildroot}$MPI_BIN
make -C $MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich_unload}

%{_mvapich2_load}
mkdir -p %{buildroot}$MPI_BIN
make -C $MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich2_unload}
%ifarch x86_64
%{_mvapich_psm_load}
mkdir -p %{buildroot}$MPI_BIN
make -C $MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich_psm_unload}

%{_mvapich2_psm_load}
mkdir -p %{buildroot}$MPI_BIN
make -C $MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich2_psm_unload}
%endif
%clean
rm -rf %{buildroot}

%files openmpi
%defattr(-, root, root, -)
%{_libdir}/openmpi/bin/*

%files mvapich
%defattr(-, root, root, -)
%{_libdir}/mvapich/bin/*

%files mvapich2
%defattr(-, root, root, -)
%{_libdir}/mvapich2/bin/*

%ifarch x86_64
%files mvapich-psm
%defattr(-, root, root, -)
%{_libdir}/mvapich-psm/bin/*

%files mvapich2-psm
%defattr(-, root, root, -)
%{_libdir}/mvapich2-psm/bin/*
%endif
%changelog
* Tue Feb 21 2012 Jay Fenlason <fenlason@redhat.com> 3.2-5.el6
- Rebuild against newer infinipath-psm, openmpi, mvapich, mvapich2
  This removes the openmpi-psm subpackage, as openmpi now switches between
   psm and non- at runtime.
- Correct version numbers for IMB and OSU benchmarkes in the description.
- Update spec file to fix pkgwrangler warnings.
  Resolves: rhbz557803
  Related: rhbz739138

* Mon Aug 22 2011 Jay Fenlason <fenlason@redhat.com> 3.2-4.el6
- BuildRequires infinipath-psm-devel for the infinipath subpackages.
  Related: rhbz725016

* Thu Aug 18 2011 Jay Fenlason <fenlason@redhat.com> 3.2-3.el6
- Build using new mvapich, mvapich2, and openmpi.
  Add support for the -psm variants of the three mpi stacks.
  Related: rhbz725016

* Fri Jan 15 2010 Doug Ledford <dledford@redhat.com> - 3.2-2.el6
- Rebuild using Fedora MPI package guidelines semantics
- Related: bz543948

* Tue Dec 22 2009 Doug Ledford <dledford@redhat.com> - 3.2-1.el5
- Update to latest release and compile against new mpi libs
- Related: bz518218

* Mon Jun 22 2009 Doug Ledford <dledford@redhat.com> - 3.1-3.el5
- Rebuild against libibverbs that isn't missing the proper ppc wmb() macro
- Related: bz506258

* Sun Jun 21 2009 Doug Ledford <dledford@redhat.com> - 3.1-2.el5
- Rebuild against MPIs that were rebuilt against non-XRC libibverbs
- Related: bz506258

* Thu Apr 23 2009 Doug Ledford <dledford@redhat.com> - 3.1-1
- Upgrade to version from ofed 1.4.1-rc3
- Related: bz459652

* Thu Sep 18 2008 Doug Ledford <dledford@redhat.com> - 3.0-2
- Add no-strict-aliasing compile flag to silence warnings

* Thu Sep 18 2008 Doug Ledford <dledford@redhat.com> - 3.0-1
- Update to latest upstream version
- Compile three times against the three mpi stacks and make three packages
- Resolves: bz451474

* Mon Jan 22 2007 Doug Ledford <dledford@redhat.com> - 2.0-2
- Recreate lost spec file and patches from memory
- Add dist tag to release
- Turn off FORTIFY_SOURCE when building

