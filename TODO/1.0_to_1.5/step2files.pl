#!/usr/bin/perl
# For extracting all exec lines from a .steprc file
# and puting them in the AfterStep 1.4 format (a file w/
# the command.
#Usage:
#   perl step2files.pl < .steprc > shell_script_name
# Now run the shell script and it will create the files for you:
#   sh shell_script_name
# Then move the files under your start menu however you want them.
#                                    
#
# By Stephan Beal (yoshiko.beal@munich.netsurf.de) 17.3.98
# Released into the Public Domain.

@LINES = <STDIN>;
foreach $line (@LINES) {
    if ( $line =~ /(Exec.*\")(.*)(\".*exec)(.*)/ ) {
        $name=$2;
        $cmd = $4;
        $cmd =~ s/^\s//;
#        print $name."=".$cmd."\n";
        print "echo '$cmd' > '$name'\n";

    }

}
