#!/usr/bin/perl
#
# $Id: pod.t 10311 2018-12-17 08:04:06Z iulius $
#
# Check all POD documents in the tree for POD formatting errors.
#
# Copyright 2016 Russ Allbery <eagle@eyrie.org>
# Copyright 2012-2014
#     The Board of Trustees of the Leland Stanford Junior University
#
# This file is part of C TAP Harness.  The current version plus supporting
# documentation is at <https://www.eyrie.org/~eagle/software/c-tap-harness/>.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT

use 5.006;
use strict;
use warnings;

use File::Spec;

# Red Hat's base perl package doesn't include Test::More (one has to install
# the perl-core package in addition).  Try to detect this and skip any Perl
# tests if Test::More is not present.
eval {
    require Test::More;
    Test::More->import();
};
if ($@) {
    print "1..0 # SKIP Test::More required for test\n"
      or croak('Cannot write to stdout');
    exit 0;
}

# Abort if C_TAP_SOURCE isn't set.
if (!$ENV{C_TAP_SOURCE}) {
    BAIL_OUT('C_TAP_SOURCE environment variable not set');
}

# Load the Test::Pod module.
if (!eval { require Test::Pod }) {
    plan(skip_all => 'Test::Pod required for testing POD');
}
Test::Pod->import;

# C_TAP_SOURCE will be the test directory.  Change to the parent.
my ($vol, $dirs) = File::Spec->splitpath($ENV{C_TAP_SOURCE}, 1);
my @dirs = File::Spec->splitdir($dirs);
pop(@dirs);
if ($dirs[-1] eq File::Spec->updir) {
    pop(@dirs);
    pop(@dirs);
}
my $root = File::Spec->catpath($vol, File::Spec->catdir(@dirs), q{});
chdir($root) or BAIL_OUT("cannot chdir to $root: $!");

# Add some additional exclusions, useful mostly for other programs that copy
# this test.
## no critic (TestingAndDebugging::ProhibitNoWarnings)
## no critic (Variables::ProhibitPackageVars)
{
    no warnings 'once';
    $Test::Pod::ignore_dirs{'.libs'} = 'libraries';
}
## use critic

# Check syntax of every POD file we can find.
all_pod_files_ok(q{.});
