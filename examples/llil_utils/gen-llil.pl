# gen-llil.pl
# Crude program to generate a big LLiL test file to use in benchmarks
# On Windows running:
#   perl gen-llil.pl big2.txt 200 3   - produces a test file with size = 35,152,000 bytes
#                                       (lines terminated with "\r\n")
#   perl gen-llil.pl big2.txt 200 3 1 - produces a test file with size = 31,636,800 bytes
#                                       (lines terminated with "\n")
# On Unix, lines are terminated with "\n" and the file size is always 31,636,800 bytes

use strict;
use warnings;
use autodie;

{
   my $ordmin = ord('a');
   my $ordmax = ord('z') + 1;

   # Generate a random word
   sub gen_random_word {
      my $word  = shift;    # word prefix
      my $nchar = shift;    # the number of random chars to append
      for my $i (1 .. $nchar) {
         $word .= chr( $ordmin + int( rand($ordmax - $ordmin) ) );
      }
      return $word;
   }
}

sub create_test_file {
   my $fname   = shift;
   my $count   = shift;
   my $wordlen = shift;
   my $fbin    = shift;
   open( my $fh_out, '>', $fname );
   $fbin and binmode($fh_out);
   for my $c ( 'aaa' .. 'zzz' ) {
      for my $i (1 .. $count) {
         print {$fh_out} gen_random_word( $c, $wordlen ) . "\t" . 1 . "\n";
      }
   }
}

my $outfile = shift;
my $count   = shift;
my $wordlen = shift;
my $fbin    = shift;    # default is to use text stream (not a binary stream)
defined($fbin) or $fbin = 0;
$outfile or die "usage: $0 outfile count wordlen\n";
$count or die "usage: $0 outfile count wordlen\n";
print "generating test file '$outfile' with count '$count' (binmode=$fbin)\n";
create_test_file($outfile, $count, $wordlen, $fbin);
print "file size=", -s $outfile, "\n";
