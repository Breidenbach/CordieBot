#!/opt/local/bin/perl

$basename ="cn";
$infilename ="allQuips.txt";
$extension = ".txt";
$countfile = "cn000.txt";
open (SOURCE, "<$infilename");
$quipCount = 0;
while (<SOURCE>)
{
	$quipCount++;
    $line = $_;
    $outfile = sprintf "%s%03s%s", $basename, $quipCount, $extension ;
	open(OUT, ">$outfile");
	print OUT "$line";
	close OUT;
}
open(OUT, ">$countfile");
print OUT "$quipCount";
close OUT;
close SOURCE;
