##############################################################################
#
#       db.linux
#       Filecheck utilty
#
#       Copyright (C) 2000 Centura Software Corporation.  All Rights Reserved.
#
##############################################################################

use FileHandle;
use Getopt::Std;

my ($result, $filename);

if ((!getopts('d:hv')) || ($opt_h) || (!@ARGV))
    {
    Usage();
	exit(($opt_h) ? 0 : 1);
    }

if (($opt_d) && ((!-d $opt_d) || (!-r $opt_d)))
	{
	printf("$0: Could not open directory \'$opt_d\'.\n");
	Usage();
	exit(1);
	}
$result = 0;
foreach $filename (@ARGV)
	{
	$result |= Read_Checklist($filename, $opt_d, $opt_v);
	}

if (!$result)
	{
	printf("Done -- all files present.\n");
	}
else
	{
	printf("Done -- one or more missing files.\n");
	}
exit($result);


sub Read_Checklist()
    {
    my ($inputfile, $rootdir, $verbose) = @_;
    my $fh, $fn, $forwardslash;
	my $result = 0;

    if (!$inputfile)
        {
        $inputfile="-";
        }
	if (($rootdir) && (!($rootdir =~ /^.*\/$/)))
		{
		$rootdir .= "/";
		}
    $fh = new FileHandle;
    open($fh, $inputfile) || (printf("$0: Could not open file \'$inputfile\'.\n"), Usage(), return(1));
	$forwardslash = (`pwd` =~ /\//);
    while (<$fh>)
        {
        s/\s*;.*//g;
        next if /^[\s]*$/;
        chomp;
		if ($forwardslash)
			{
			s:\\:/:g;
			}
		else
			{
			s:/:\\:g;
			}
		$fn = $rootdir . $_;
        if (-e $fn)
            {
            printf("$fn [ok]\n") if ($verbose);
            }
        else
            {
			$result = 1;
            printf( "$fn [missing]\n");
            }
            
        }
    close($fh);
	return($result);
    }

sub Usage()
    {
    printf("\nUsage: perl $0 [-d dir] [-h] [-v] file [file...]\n\nwhere:\n");
    printf("\t-d dir\tspecifies \'dir\' as root installation directory.  Uses current\n\t\tdirectory by default.\n");
    printf("\t-h\tDisplay this message.\n");
    printf("\t-v\tVerbose: list ALL files and their status.\n");
    printf("\tfile\tRead packing list from one or more \'files\'.  Use \'-\' to\n\t\tread from stdin.\n");
    }
