#!/usr/bin/env perl
use strict;
use warnings;
use List::Util 'shuffle';
my @arr = shuffle <>;
print @arr;
