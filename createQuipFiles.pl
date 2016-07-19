#!/opt/local/bin/perl

# remove old .txt files prior to creating them

$basename ="nc";
$infilename ="allQuips.txt";
$extension = ".txt";
$countfile = $basename . "000" . $extension;
$remfiles = $basename . "*" . $extension;

unlink glob "$remfiles";
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
