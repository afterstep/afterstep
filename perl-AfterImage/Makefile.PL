use 5.005;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

my $afterimage_config = 'afterimage-config';
if (-x '../libAfterImage/afterimage-config') {
	$afterimage_config = '../libAfterImage/afterimage-config';
}

my $cflags = backticks($afterimage_config, '--cflags');
my $libs = backticks($afterimage_config, '--libs');

WriteMakefile(
    NAME              => 'AfterImage',
    VERSION_FROM      => 'lib/AfterImage.pm',
		PMLIBDIRS         => [ qw(lib lib/AfterImage) ],
    PREREQ_PM         => {},
    ($] >= 5.005 ?
      (ABSTRACT_FROM  => 'lib/AfterImage.pm',
       AUTHOR         => 'Ethan Fischer <allanon@crystaltokyo.com>') : ()),
    LIBS              => [$libs],
    DEFINE            => '',
    INC               => $cflags,
);

sub backticks {
	my $pid = open(KID_TO_READ, "-|");
	die "can't fork: $!" if !defined $pid;
	if ($pid) {
		local($/);
		my $str = <KID_TO_READ>;
		close(KID_TO_READ);
		return $str;
	} else {
		($EUID, $EGID) = ($UID, $GID);
		exec(@_) || die "can't exec program $_[0]: $!";
	}
	# NOTREACHED
}
