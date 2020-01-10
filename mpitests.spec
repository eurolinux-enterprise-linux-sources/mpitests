Summary: MPI Benchmarks and tests
Name: mpitests
Version: 4.1
Release: 1%{?dist}
License: CPL and BSD
Group: Applications/Engineering
# These days we get the benchmark soucres from Intel and OSU directly
# rather than from openfabrics.
URL: http://www.openfabrics.org
# https://software.intel.com/en-us/articles/intel-mpi-benchmarks
Source0: https://software.intel.com/sites/default/files/managed/a3/b2/IMB_4.1.tgz
Source1: http://mvapich.cse.ohio-state.edu/download/mvapich/osu-micro-benchmarks-5.3.tar.gz
Patch0: 0001-imb-fix-Makefiles.patch
Patch1: 0001-mpi-collective-no-async-ops-MPI_I.patch
BuildRequires: hwloc-devel, libibmad-devel

%description
Set of popular MPI benchmarks:
IMB-4.1
OSU benchmarks ver 5.3

%package openmpi
Summary: MPI tests package compiled against openmpi
Group: Applications
BuildRequires: openmpi-devel >= 1.4
%description openmpi
MPI test suite compiled against the openmpi package

%package mpich
Summary: MPI tests package compiled against mpich
Group: Applications
BuildRequires: mpich-devel >= 3.0.4
BuildRequires: librdmacm-devel, libibumad-devel
%description mpich
MPI test suite compiled against the mpich package

%package mpich32
Summary: MPI tests package compiled against mpich-3.2
Group: Applications
BuildRequires: mpich-3.2-devel
BuildRequires: librdmacm-devel, libibumad-devel
%description mpich32
MPI test suite compiled against the mpich-3.2 package

# mvapich2 is not yet built on s390(x)
%ifnarch s390 s390x
%package mvapich2
Summary: MPI tests package compiled against mvapich2
Group: Applications
BuildRequires: mvapich2-devel >= 1.4
BuildRequires: librdmacm-devel, libibumad-devel
%description mvapich2
MPI test suite compiled against the mvapich2 package

%package mvapich222
Summary: MPI tests package compiled against mvapich2-2.2
Group: Applications
BuildRequires: mvapich2-2.2-devel
BuildRequires: librdmacm-devel, libibumad-devel
%description mvapich222
MPI test suite compiled against the mvapich2-2.2 package

# s390(x) did not have openmpi-1.6
%package compat-openmpi16
Summary: MPI tests package compiled against compat-openmpi16
Group: Applications
BuildRequires: compat-openmpi16-devel
%description compat-openmpi16
MPI test suite compiled against the compat-openmpi16 package
%endif

# PSM is x86_64-only
%ifarch x86_64
%package mvapich2-psm
Summary: MPI tests package compiled against mvapich2 using InfiniPath
Group: Applications
BuildRequires: mvapich2-psm-devel >= 1.4
BuildRequires: librdmacm-devel, libibumad-devel
BuildRequires: infinipath-psm-devel
%description mvapich2-psm
MPI test suite compiled against the mvapich2 package using InfiniPath

%package mvapich222-psm
Summary: MPI tests package compiled against mvapich2-2.2 using InfiniPath
Group: Applications
BuildRequires: mvapich2-2.2-psm-devel
BuildRequires: librdmacm-devel, libibumad-devel
BuildRequires: infinipath-psm-devel
%description mvapich222-psm
MPI test suite compiled against the mvapich2-2.2 package using InfiniPath

%package mvapich222-psm2
Summary: MPI tests package compiled against mvapich2-2.2 using OmniPath
Group: Applications
BuildRequires: mvapich2-2.2-psm2-devel
BuildRequires: librdmacm-devel, libibumad-devel
BuildRequires: libpsm2-devel
%description mvapich222-psm2
MPI test suite compiled against the mvapich2-2.2 package using OmniPath
%endif

%prep
%setup -c 
%setup -T -D -a 1
cd imb
%patch0 -p1 -b.make
cd ../osu-micro-benchmarks-5.3
%patch1 -p1 -b.noasync
cd ..

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
  cd .$MPI_COMPILER/imb/src
  make -f make_mpich OPTFLAGS="%{optflags}" IMB-MPI1 IMB-EXT IMB-IO
  cd ../../osu-micro-benchmarks-5.3
  %configure
  make %{?_smp_mflags}
  cd ../..
}

# do N builds, one for each mpi stack
%{_openmpi_load}
do_build
%{_openmpi_unload}

%{_mpich_load}
do_build
%{_mpich_unload}

%{_mpich_3_2_load}
do_build
%{_mpich_3_2_unload}

%ifnarch s390 s390x
%{_mvapich2_load}
do_build
%{_mvapich2_unload}

%{_mvapich2_2_2_load}
do_build
%{_mvapich2_2_2_unload}

%{_compat_openmpi16_load}
do_build
%{_compat_openmpi16_unload}
%endif

%ifarch x86_64
%{_mvapich2_psm_load}
do_build
%{_mvapich2_psm_unload}

%{_mvapich2_2_2_psm_load}
do_build
%{_mvapich2_2_2_psm_unload}

%{_mvapich2_2_2_psm2_load}
do_build
%{_mvapich2_2_2_psm2_unload}
%endif

%install
do_install() {
  mkdir -p %{buildroot}$MPI_BIN
  cd .$MPI_COMPILER
  for X in allgather allgatherv allreduce alltoall alltoallv barrier bcast gather gatherv reduce reduce_scatter scatter scatterv; do
    cp osu-micro-benchmarks-5.3/mpi/collective/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in acc_latency get_bw get_latency put_bibw put_bw put_latency; do
    cp osu-micro-benchmarks-5.3/mpi/one-sided/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in bibw bw latency latency_mt mbw_mr multi_lat; do
    cp osu-micro-benchmarks-5.3/mpi/pt2pt/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in EXT IO MPI1; do
    cp imb/src/IMB-$X %{buildroot}$MPI_BIN/mpitests-IMB-$X
  done
  cd ..
}

# do N installs, one for each mpi stack
%{_openmpi_load}
do_install
%{_openmpi_unload}

%{_mpich_load}
do_install
%{_mpich_unload}

%{_mpich_3_2_load}
do_install
%{_mpich_3_2_unload}

%ifnarch s390 s390x
%{_mvapich2_load}
do_install
%{_mvapich2_unload}

%{_mvapich2_2_2_load}
do_install
%{_mvapich2_2_2_unload}

%{_compat_openmpi16_load}
do_install
%{_compat_openmpi16_unload}
%endif

%ifarch x86_64
%{_mvapich2_psm_load}
do_install
%{_mvapich2_psm_unload}

%{_mvapich2_2_2_psm_load}
do_install
%{_mvapich2_2_2_psm_unload}

%{_mvapich2_2_2_psm2_load}
do_install
%{_mvapich2_2_2_psm2_unload}
%endif

%files openmpi
%{_libdir}/openmpi/bin/*

%files mpich
%{_libdir}/mpich/bin/*

%files mpich32
%{_libdir}/mpich-3.2/bin/*

%ifnarch s390 s390x
%files mvapich2
%{_libdir}/mvapich2/bin/*

%files mvapich222
%{_libdir}/mvapich2-2.2/bin/*

%files compat-openmpi16
%{_libdir}/compat-openmpi16/bin/*
%endif

%ifarch x86_64
%files mvapich2-psm
%{_libdir}/mvapich2-psm/bin/*

%files mvapich222-psm
%{_libdir}/mvapich2-2.2-psm/bin/*

%files mvapich222-psm2
%{_libdir}/mvapich2-2.2-psm2/bin/*
%endif

%changelog
* Tue Jul 05 2016 Michal Schmidt <mschmidt@redhat.com> - 4.1-1
- Update IMB to 4.1.
- Update OSUMB to 5.3.
- Add subpackages for new mpich and mvapich2 versions and variants.
- Resolves: rhbz961885
- Resolves: rhbz1093466

* Mon Sep 07 2015 Michal Schmidt <mschmidt@redhat.com> - 3.2-14
- Rebuild for new openmpi. Add compat-openmpi16 subpackage.
- Resolves: rhbz1258866

* Thu Sep 11 2014 Yaakov Selkowitz <yselkowi@redhat.com> - 3.2-13
- Enable on aarch64
  Resolves: rhbz1100503

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

