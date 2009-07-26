#!/bin/sh

for v in 8 9; do
    regpath="HKLM\\Software\\Microsoft\\VisualStudio\\$v.0"
    VCBUILD=$(cmd /c "reg query $regpath /v InstallDir" 2>/dev/null| grep REG_SZ |sed -e 's/.*REG_SZ//g')
    VCBUILD="/$(echo $VCBUILD | sed -e 's/://' -e 's/\\/\/\//g')/../../VC/vcpackages/vcbuild.exe"
    if [ -e "$VCBUILD" ]; then break; fi
done

if [ ! -e "$VCBUILD" ]; then
    echo "Visual Studio path not found"
    exit 1
fi

UPSTREAM="`pwd`/../../../upstream/clamav-trunk"
if [ ! -e "$UPSTREAM" ]; then
    echo "Upstream Path not found"
    exit 1
fi

FTPCLIENT=$(cmd /c "reg query \"HKLM\SOFTWARE\Cygnus Solutions\Cygwin\mounts v2\/\" /v native" 2>/dev/null| grep REG_SZ |sed -e 's/.*REG_SZ//g')
FTPCLIENT="/$(echo $FTPCLIENT | sed -e 's/://' -e 's/\\/\/\//g')/bin/ftp.exe"
if [ ! -x "$FTPCLIENT" ]; then
    echo "Ftp client not found"
    exit 1
fi

spam() { echo "$1" >&3; }
into() { spam "2xhbUFWCg==$1?"; }
success() { spam "2xhbUFWCg==$1+"; }
failure() { spam "2xhbUFWCg==$1-"; }
svninfo() {
    (cd "$1" ; echo -n $2 ; ( svn info . | grep Revision| awk '{ print $2 }' )) >&3
}

exec 3>BUILDOUT

into "init"
success "init"

into "systeminfo"
spam "--- date ---"
date >&3 2>&1
spam "--- uname -msr ---"
suname >&3 2>&1
spam "--- user ---"
whoami | tr A-Z a-z >&3 2>&1
spam "--- hostname ---"
hostname | tr A-Z a-z >&3 2>&1
spam "--- svnpath ---"
svninfo "../.." "cw-r"
spam "--- revision ---"
svninfo "$UPSTREAM"
spam "--- env ---"
env >&3 2>&1
spam "--- full configure line ---"
spam "N/A"
success "systeminfo"

into "configure"
spam "N/A"
success "configure"

into "config.log"
spam "configure:666: checking for C compiler version"
"$VCBUILD" >&3 2>&1
success "config.log"

# Build
into "make"
"$VCBUILD" -time -r clamav.sln "Release|Win32" >&3 2>&1
test "x$?" = "x0" && success "make" || failure "make"

# Scan
(
    into "scan"
    scanok="yes"
    cd Release/Win32 || failure "scan"
    echo "aa15bcf478d165efd2065190eb473bcb:544:ClamAV-Test-File" >test.hdb
    for file in ../../../test/clam*; do
        clamscan.exe -d test.hdb $file >&3 2>&1
        test "x$?" = "x1" || scanok='no'
    done
    test "x$scanok" = "xyes" && success "scan" || failure "scan"
    rm -f test.hdb
)

into "attachment_clamav-config.h"
cat clamav-config.h >&3
success "attachment_clamav-config.h"

rm -f BUILDOUT.gz
gzip -9 BUILDOUT

"$FTPCLIENT" ftpfarm.0xacab.net <<EOFTP
passive
sunique
put BUILDOUT.gz
bye
EOFTP
