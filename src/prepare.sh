#!/bin/bash
set -x
SPWD=`pwd`

if [ `cat /etc/issue | grep -ic suse` -eq 1 ]; then
    RPMHOME=/usr/src/packages
else
    RPMHOME=/usr/src/redhat
fi

version=3.2
name=mpitests
# test list
TEST1=osu_benchmarks-3.1.1
TEST2=IMB-3.2
TEST3=presta-1.4.0
SPECFILE=mpitests.spec
SVNLOC=svn://mtls50/svn.mpi/trunk/ibed/mpitests
topdir=/tmp/rpms

# cd $RPMHOME/SOURCES/
cd /tmp
# svn export $SVNLOC $name-$version
revision=`(svn export $SVNLOC $name-$version | grep "Exported revision" | awk '{print $3}' | tr -d ".")`
perl -pi -e "s,TEST_REVISION,$revision,g" $name-$version/mpitests.spec
tar czvf $name-$version.tar.gz $name-$version 

mkdir -p ${topdir}/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
name=`rpmbuild -ts --define "_topdir ${topdir}" $name-$version.tar.gz | tail -1 | awk '{print $2}'`
mv -f $name $SPWD/
echo `basename $name` > $SPWD/latest.txt

# cd $RPMHOME/SOURCES/ \
# && tar czvf $name-$version.tar.gz $name-$version \
# && cd - \
# && cp $RPMHOME/SOURCES/$name-$version/$SPECFILE $RPMHOME/SPECS/ \
# && rm -rf $RPMHOME/SOURCES/$name-$version 
#
# echo Please run follow commands:
# echo "cd $RPMHOME/SPECS && rpmbuild -bs $SPECFILE"
