Summary: MPI Benchmarks and tests
Name: mpitests
Version: 3.2.4
Release: 2%{?dist}
License: BSD
Group: Applications/Engineering
# These days we get the benchmark soucres from Intel and OSU directly
# rather than from openfabrics.
URL: http://www.openfabrics.org
# https://software.intel.com/en-us/articles/intel-mpi-benchmarks
# https://software.intel.com/sites/default/files/article/157859/imb-3.2.4-updated.tgz
Source0: imb-3.2.4-updated.tgz
# http://mvapich.cse.ohio-state.edu/benchmarks/osu-micro-benchmarks-4.3.tar.gz
Source1: osu-micro-benchmarks-4.3.tar.gz
Patch0: mpitests-3.2-make.patch
Provides: mpitests
BuildRequires: hwloc-devel, libibmad-devel
# mvapich2 only exists on these arches
ExclusiveArch: i686 i386 x86_64 ia64

%description
Set of popular MPI benchmarks:
IMB-3.2.4
OSU benchmarks ver 4.3

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

%package mvapich
Summary: MPI tests package compiled against mvapich
Group: Applications
BuildRequires: mvapich-devel >= 1.2.0
BuildRequires: librdmacm-devel, libibumad-devel
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
BuildRequires: librdmacm-devel, libibumad-devel
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
%setup -c 
%setup -T -D -a 1
%patch0 -b.make

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
  cd .$MPI_COMPILER/imb/3.2.4/src
  make -f make_mpich OPTFLAGS="%{optflags}"
  cd ../../../osu-micro-benchmarks-4.3
  %configure
  make %{?_smp_mflags}
  cd ../..
}
do_mvapich_build() { 
  mkdir .$1
  cp -al * .$1
  cd .$1/imb/3.2.4/src
  make -f make_mpich OPTFLAGS="%{optflags}" IMB-MPI1
  cd ../../../osu-micro-benchmarks-4.3
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

%{_mvapich_load}
do_mvapich_build mvapich
%{_mvapich_unload}

%{_mvapich2_load}
do_build
%{_mvapich2_unload}

%ifarch x86_64
%{_mvapich_psm_load}
do_mvapich_build mvapich-psm
%{_mvapich_psm_unload}

%{_mvapich2_psm_load}
do_build
%{_mvapich2_psm_unload}
%endif

%install
do_install() {
  mkdir -p %{buildroot}$MPI_BIN
  cd .$MPI_COMPILER
  for X in allgather allgatherv allreduce alltoall alltoallv barrier bcast gather gatherv reduce reduce_scatter scatter scatterv; do
    cp osu-micro-benchmarks-4.3/mpi/collective/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in acc_latency get_bw get_latency put_bibw put_bw put_latency; do
    cp osu-micro-benchmarks-4.3/mpi/one-sided/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in bibw bw latency latency_mt mbw_mr multi_lat; do
    cp osu-micro-benchmarks-4.3/mpi/pt2pt/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in EXT IO MPI1; do
    cp imb/3.2.4/src/IMB-$X %{buildroot}$MPI_BIN/mpitests-IMB-$X
  done
  cd ..
}
do_mvapich_install() {
  mkdir -p %{buildroot}$MPI_BIN
  cd .$1
  for X in allgather allgatherv allreduce alltoall alltoallv barrier bcast gather gatherv reduce reduce_scatter scatter scatterv; do
    cp osu-micro-benchmarks-4.3/mpi/collective/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  for X in bibw bw latency mbw_mr multi_lat; do
    cp osu-micro-benchmarks-4.3/mpi/pt2pt/osu_$X %{buildroot}$MPI_BIN/mpitests-osu_$X
  done
  cp imb/3.2.4/src/IMB-MPI1 %{buildroot}$MPI_BIN/mpitests-IMB-MPI1
  cd ..
}

rm -rf %{buildroot}
# do N installs, one for each mpi stack
%{_openmpi_load}
do_install
%{_openmpi_unload}

%{_mpich_load}
do_install
%{_mpich_unload}

%{_mvapich_load}
do_mvapich_install mvapich
%{_mvapich_unload}

%{_mvapich2_load}
do_install
%{_mvapich2_unload}

%ifarch x86_64
%{_mvapich_psm_load}
do_mvapich_install mvapich-psm
%{_mvapich_psm_unload}

%{_mvapich2_psm_load}
do_install
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
* Wed Jun 11 2014 Jay Fenlason <fenlason@redhat.com> 3.2.4-2
- Put back the mpich{,-psm} subpackages, which we still build on RHEL-6
  Resolves: rhbz1093468

* Wed Jun 11 2014 Jay Fenlason <fenlason@redhat.com> 3.2.4-1
- Backport from RHEL-7.0
- Edit old changelog entries so commit can work.
- Update to latest imb and osu versions, no longer using openfabrics.org
  tarball.
  Resolves: rhbz1093468

* Mon Jan 6 2014 Jay Fenlason <fenlason@redhat.com) - 3.2-11
- Fix flags regexes
  R e s o l v e s: rhbz 1048884

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com>
- Mass rebuild 2013-12-27

* Wed Oct 9 2013 Jay Fenlason <fenlason@redhat.com> 3.2-10
- Add BuildRequires for libibmad-devel
- Build against mpich, now that we have a mpi-3 version of it.
- rebuild against new mvapich2
  R e s o l v e s: rhbz1011910, rhbz1011915

* Tue Aug 27 2013 Jay Fenlason <fenlason@redhat.com> 3.2-9
- Remove presta and obsolete versions of imb and osu from the tarball.
- clean up build process

* Thu Aug 22 2013 Jay Fenlason <fenlason@redhat.com> 3.2-8
- Remove mvapich, as it is dead upstream

* Tue Feb 26 2013 Jay Fenlason <fenlason@redhat.com> 3.2-7
- Add win_free patch to close
  R e s o l v e s: rhbz834237

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

