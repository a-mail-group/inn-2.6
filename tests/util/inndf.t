#! /bin/sh
# $Id: inndf.t 10113 2016-11-06 14:17:37Z iulius $
#
# Test suite for inndf.

UNAME_SYSTEM=`(uname -s) 2>/dev/null`

# The count starts at 1 and is updated each time ok is printed.  printcount
# takes "ok" or "not ok".
count=1
printcount () {
    echo "$1 $count $2"
    count=`expr $count + 1`
}

# Find the right directory.
inndf="../../backends/inndf"
makehistory="../../expire/makehistory"
dirs='../data data tests/data'
for dir in $dirs ; do
    if [ -r "$dir/articles/1" ] ; then
        cd $dir
        break
    fi
done
if [ ! -x "$inndf" ] ; then
    echo "Could not find inndf" >&2
    exit 1
fi

# Print out the count of tests.
echo 6

# Make sure df -k works, or we have to just skip this test.
if df -k . > /dev/null 2>&1 ; then
    if [ -z "${UNAME_SYSTEM##IRIX[[:alnum:]]*}" ] ; then
        real=`df -k . | sed 1d | tr -d '\r\n' | awk '{ print $5 }'`
    else
        real=`df -k . | sed 1d | tr -d '\r\n' | awk '{ print $4 }'`
    fi
    try=`$inndf .`
    diff=`expr "$real" - "$try"`
    if [ "$diff" -gt 30000 ] || [ "$diff" -lt -30000 ] ; then
        printcount "not ok"
    else
        printcount "ok"
    fi
else
    printcount "ok" "# skip"
fi

# Make sure df -i works, or we have to just skip this test.  Also accept a
# return value of 2^31 - 1 or 2^32 - 1 from inndf regardless of what df says,
# since this is what Reiser and some other file systems return in some
# versions of Linux.
if df -i . > /dev/null 2>&1 ; then
    if [ -z "${UNAME_SYSTEM##IRIX[[:alnum:]]*}" ] ; then
        real=`df -i . | sed 1d | tr -d '\r\n' | awk '{ print $8 }'`
    elif [ "${UNAME_SYSTEM}" = "FreeBSD" ] \
      || [ "${UNAME_SYSTEM}" = "NetBSD" ] \
      || [ "${UNAME_SYSTEM}" = "Darwin" ] ; then
        real=`df -i . | sed 1d | tr -d '\r\n' | awk '{ print $7 }'`
    else
        real=`df -i . | sed 1d | tr -d '\r\n' | awk '{ print $4 }'`
    fi
    try=`$inndf -i .`
    if [ "$try" = 2147483647 ] || [ "$try" = 4294967295 ] ; then
        printcount "ok"
    else
        diff=`expr "$real" - "$try"`
        if [ "$diff" -gt 5000 ] || [ "$diff" -lt -5000 ] ; then
            printcount "not ok"
        else
            printcount "ok"
        fi
    fi
else
    printcount "ok" "# skip"
fi

# Create a message spool so that we can use makehistory to generate an
# overview database.
mkdir -p spool/example/config
mkdir -p spool/example/test
cp articles/1 spool/example/test/1
cp articles/2 spool/example/config/1
cp articles/3 spool/example/test/2
cp articles/4 spool/example/test/3
rm -f spool/example/config/2
ln -s ../test/2 spool/example/config/2

# First, generate a tradindexed overview to test inndf -n and make sure that
# inndf -o returns the appropriate thing.
INN_TESTSUITE=1; export INN_TESTSUITE
INNCONF="etc/inn-tdx.conf"; export INNCONF
mkdir -p ov-tmp tmp run
if ! $makehistory -x -O > /dev/null 2>&1 ; then
    echo "makehistory failed, unable to continue" >&2
    exit 1
fi
out=`$inndf -n`
if [ "$out" = "5 overview records stored" ] ; then
    printcount "ok"
else
    printcount "not ok"
fi
out=`$inndf -o`
if [ "$out" = "Space used is meaningless for the tradindexed method" ] ; then
    printcount "ok"
else
    printcount "not ok"
fi

# Delete that overview and then create a buffindexed overview, testing both
# inndf -n and inndf -o.
rm -rf ov-tmp
mkdir ov-tmp
dd if=/dev/zero of=ov-tmp/buffer bs=1024k count=1 > /dev/null 2>&1
INNCONF="etc/inn-bfx.conf"; export INNCONF
if ! $makehistory -x -O > /dev/null 2>&1 ; then
    echo "makehistory failed, unable to continue" >&2
    exit 1
fi
out=`$inndf -n`
if [ "$out" = "5 overview records stored" ] ; then
    printcount "ok"
else
    echo "$out"
    printcount "not ok"
fi
out=`$inndf -o`
if [ "$out" = "3.00% overview space used" ] ; then
    printcount "ok"
else
    echo "$out"
    printcount "not ok"
fi

# Clean up.
rm -rf spool ov-tmp tmp run db/group.index
