Summary: MPI Benchmarks and tests
Name: mpitests
Version: 3.2
Release: 12%{?dist}
License: BSD
Group: Applications/Engineering
# We get the mpitests.tar.gz file from an OFED release.
# Unfortunately, they're not good about changing the name
# of the tarball when they change the contents.
# and we had to do some cleanup on the contents.
URL: http://www.openfabrics.org
Source: mpitests-%{version}-rh.tar.gz
Patch0: mpitests-3.2-make.patch
Patch1: mpitests-win-free.patch
Provides: mpitests
BuildRequires: hwloc-devel, libibmad-devel
# mvapich2 only exists on these arches
ExclusiveArch: i686 i386 x86_64 ia64 ppc64le

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

%package mpich
Summary: MPI tests package compiled against mpich
Group: Applications
BuildRequires: mpich-devel >= 3.0.4
BuildRequires: librdmacm-devel, libibumad-devel
%description mpich
MPI test suite compiled against the mpich package

%package mvapich2
Summary: MPI tests package compiled against mvapich2
Group: Applications
BuildRequires: mvapich2-devel >= 1.4
BuildRequires: librdmacm-devel, libibumad-devel
%description mvapich2
MPI test suite compiled against the mvapich2 package

%ifarch x86_64
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
%setup -q
# secretly patch the code one layer down, not at the top level
%patch0 -p1 -b .make
%patch1 -p1 -b .win_free

%build
RPM_OPT_FLAGS=`echo $RPM_OPT_FLAGS | sed -e 's/-Wp,-D_FORTIFY_SOURCE=.//' -e 's/-fstack-protector-strong//' -e 's/-fstack-protector//'`
# We don't do a non-mpi version of this package, just straight to the mpi builds
export CC=mpicc
export CXX=mpicxx
export FC=mpif90
export F77=mpif77
export CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing"
do_build() { 
  mkdir .$MPI_COMPILER
  cp -al * .$MPI_COMPILER
  cd .$MPI_COMPILER
  make $*
  cd ..
}


# do N builds, one for each mpi stack
%{_openmpi_load}
do_build all
%{_openmpi_unload}

%{_mpich_load}
do_build all
%{_mpich_unload}

%{_mvapich2_load}
do_build all
%{_mvapich2_unload}
%ifarch x86_64

%{_mvapich2_psm_load}
do_build all
%{_mvapich2_psm_unload}
%endif
%install
rm -rf %{buildroot}
# do N installs, one for each mpi stack
%{_openmpi_load}
mkdir -p %{buildroot}$MPI_BIN
make -C .$MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_openmpi_unload}

%{_mpich_load}
mkdir -p %{buildroot}$MPI_BIN
make -C .$MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mpich_unload}

%{_mvapich2_load}
mkdir -p %{buildroot}$MPI_BIN
make -C .$MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich2_unload}
%ifarch x86_64
%{_mvapich2_psm_load}
mkdir -p %{buildroot}$MPI_BIN
make -C .$MPI_COMPILER DESTDIR=%{buildroot} INSTALL_DIR=$MPI_BIN install
%{_mvapich2_psm_unload}
%endif
%clean
rm -rf %{buildroot}

%files openmpi
%defattr(-, root, root, -)
%{_libdir}/openmpi/bin/*

%files mpich
%defattr(-, root, root, -)
%{_libdir}/mpich/bin/*

%files mvapich2
%defattr(-, root, root, -)
%{_libdir}/mvapich2/bin/*

%ifarch x86_64
%files mvapich2-psm
%defattr(-, root, root, -)
%{_libdir}/mvapich2-psm/bin/*
%endif
%changelog
* Tue Sep 9 2014 Dan Horák <dhorak@redhat.com) - 3.2-12
- enable on ppc64le
- Resolves:#1125616

* Mon Jan 6 2014 Jay Fenlason <fenlason@redhat.com) - 3.2-11
- Fix flags regexes
  Resolves: rhbz1048884

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com>
- Mass rebuild 2013-12-27

* Wed Oct 9 2013 Jay Fenlason <fenlason@redhat.com> 3.2-10
- Add BuildRequires for libibmad-devel
- Build against mpich, now that we have a mpi-3 version of it.
- rebuild against new mvapich2
  Resolves: rhbz1011910, rhbz1011915

* Tue Aug 27 2013 Jay Fenlason <fenlason@redhat.com> 3.2-9
- Remove presta and obsolete versions of imb and osu from the tarball.
- clean up build process

* Thu Aug 22 2013 Jay Fenlason <fenlason@redhat.com> 3.2-8
- Remove mvapich, as it is dead upstream

* Tue Feb 26 2013 Jay Fenlason <fenlason@redhat.com> 3.2-7
- Add win_free patch to close
  Resolves: rhbz834237

* Thu May 31 2012 Jay Fenlason <fenlason@redhat.com> 3.2-6
- Add BuildRequires: hwloc-devel to close
  Resolves: rhbz822526

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

