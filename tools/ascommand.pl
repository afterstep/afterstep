#!/usr/bin/perl

#
# Copyright (C) 1998, 1999 Ethan Fischer <allanon@crystaltokyo.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

require 5.002;
use Socket;
use strict;

# AfterStep module packet format
# ------------------------------
#
# Packets are arrays of unsigned long, with a header of three elements.
# header[0] == START_FLAG (== 0xffffffff)
# header[1] == event type (as defined in include/module.h, and requested 
#                          with a SET_MASK module command)
# header[2] == number of elements in the body of the packet
#
# The content of the body of the packet is variable, depending on the 
# event type.

# get AfterStep socket name (via xprop)

sub module_get_socket_name {

   #won't work for me. -Vae
  my $socket_name = "$ENV{'HOME'}/.afterstep/connect.DISPLAY=$ENV{'DISPLAY'}";
  
  my $xprop_root;

  open(XPROP, "xprop -root |") || warn "unable to execute xprop: $!";

  while (<XPROP>) {
    if (/^_NET_SUPPORTING_WM_CHECK\(WINDOW\)/) {
      $xprop_root = (split(/#/))[1];
      last;
    }
  }
  close(XPROP);

  open (XPROP, "xprop -id $xprop_root |") || warn "unable to execute xprop: $!";
  while (<XPROP>) {
    if (/^_AS_MODULE_SOCKET\(STRING\)/) {
      $socket_name = (split(/"/))[1];
      last;
    }
  }
  close(XPROP);
  


  return $socket_name;
}

# connect to AfterStep via a UNIX-domain socket

sub module_connect {
  my $socket_name = $_[0];
  local *SOCKET;

  socket(SOCKET, PF_UNIX, SOCK_STREAM, 0)       || warn "socket: $!";
  connect(SOCKET, sockaddr_un($socket_name))    || warn "connect: $!";

  # set unbuffered I/O
  select((select(SOCKET), $| = 1)[0]);

  return *SOCKET;
}

# send an AfterStep module command
# $_[0] == socket to write command to
# $_[1] == message to send

sub module_send {
  my ($fh, $linelen) = ($_[0], length($_[2]));
  print $fh pack("LLa${linelen}L", $_[1], $linelen, $_[2], 1);
}

# read an AfterStep module packet
# $_[0] == socket to read packet from
# returns packet

sub module_read {
  my ($line, @packet, $packetlen);
  for ($line = "" ; length($line) < 12 ; ) {
    my $subline;
    if (sysread($_[0], $subline, 12 - length($line)) > 0) {
      $line = join("", $line, $subline);
    }
  }
  @packet = unpack("L3", $line);
  $packetlen = $packet[2] - 3;
  for ($line = "" ; length($line) < 4 * $packetlen ; ) {
    my $subline;
    if (sysread($_[0], $subline, 4 * $packetlen - length($line)) > 0) {
      $line = join("", $line, $subline);
    }
  }
  splice(@packet, 3, 0, unpack("L$packetlen", $line));
  return @packet;
}

# poll a socket for input until timeout expires
# $_[0] == timeout
# $_[1]... == socket(s) to check
# returns 1 if input is ready, else 0

sub socket_poll_input {
  my ($timeout) = ($_[0]);
  my $rin = pack("C", 0);
  shift @_;
  for (@_) {
    vec($rin, fileno($_), 1) = 1;
  }
  return scalar select($rin, undef, undef, $timeout);
}

# an example of how to write a function that watches the module socket 
# for AfterStep events
# $_[0] == module socket to communicate with
# $_[1] == socket commands will be read from
# $_[2] == socket AfterStep output will be echoed to
# $_[3] == prompt for input (boolean)
# this function never returns

sub module_loop {
  my ($as_socket, $isock, $osock, $prompt) = ($_[0], $_[1], $_[2], $_[3]);
  select($osock);
  $| = 1;
  while (1) {
    print "> " if $prompt;
    socket_poll_input(undef, $as_socket, $isock);
    print "module_read\n" if $prompt && socket_poll_input(0, $as_socket);
    if (socket_poll_input(0, $isock)) {
      my $str;
      if (defined(sysread($isock, $str, 1000))) {
        chomp($str);
        module_send($as_socket, 0, $str) if length($str);
      }
    }
    while (socket_poll_input(0, $as_socket)) {
      my ($i, @packet);
      @packet = module_read($as_socket);
      printf("%08x %08x %08x ", $packet[0], $packet[1], $packet[2]);
      for ($i = 0 ; $i < $packet[2] ; $i++) {
        if (defined($packet[3+$i])) {
          printf("%08x ", $packet[3+$i]);
        }
      }
      print "\n";
    }
  }
}

# local variables
my $name = (reverse split /\//, $0)[0];
my $version = "1.2";
my ($file, $interactive, $window_id);
my $module_socket;

sub version {
  print "ascommand.pl version $version\n";
}

sub usage {
  print "Usage:\n";
  print "$name [-f file] [-h] [-i] [-v] [-w id] [--] [command]\n";
  print "  -f --file        input commands from file (- means stdin)\n";
  print "  -h --help        this help\n";
  print "  -i --interactive starts interactive communication with AfterStep\n";
  print "  -v --version     print version information\n";
  print "  -w --window-id   window id to send to AfterStep (in hex)\n";
  print "  --               end parsing of command line options\n";
  print "  command          command to send to AfterStep\n";
  print "If -f - or -i is specified, $name will read commands from standard input,\n";
  print "and print results on standard output.  -i is noisier than -f -.\n";
}

# check dependencies
if (!defined($ENV{'DISPLAY'})) {
  print "$name: DISPLAY environment variable must be set (and valid)\n";
  exit 1;
}

# get options
$window_id = 0;
while (defined $ARGV[0]) {
  my $arg = shift;
  if (($arg eq "-f" || $arg eq "--file") && scalar(@ARGV) > 0) {
    $file = shift;
  } elsif ($arg eq "-h" || $arg eq "--help") {
    version();
    usage();
    exit 0;
  } elsif ($arg eq "-i" || $arg eq "--interactive") {
    $interactive = 1;
  } elsif ($arg eq "-v" || $arg eq "--version") {
    version();
    exit 0;
  } elsif (($arg eq "-w" || $arg eq "--window-id") && scalar(@ARGV) > 0) {
    $window_id = hex shift;
  } elsif ($arg eq "--") {
    last;
  } elsif ($arg =~ "-.+") {
    print "$name: unknown option '$arg'\n";
    exit 1;
  } else {
    unshift(@ARGV, $arg);
    last;
  }
}

# need a command if interactive wasn't specified
if (!defined($interactive) && !defined($file) && !defined($ARGV[0])) {
  print "$name: a file or command is required in non-interactive mode\n";
  exit 1;
}

# connect to AfterStep
$module_socket = module_connect(module_get_socket_name());

# report our name
module_send($module_socket, 0, "SET_NAME $name");

# send a single user-requested command
if (defined($ARGV[0])) {
  module_send($module_socket, $window_id, $ARGV[0]);
}

# send a list of commands
if (defined($file) && $file ne "-") {
  open(FILE, $file);
  while (<FILE>) {
    module_send($module_socket, $window_id, $_);
  }
  close(FILE);
}

# be a true module, and open an interactive channel to AS
if (defined($file) && $file eq "-") {
  module_loop($module_socket, *STDIN, *STDOUT, 0);
}

# be really noisily interactive (assume there's a user on the other end)
if (defined($interactive)) {
  module_loop($module_socket, *STDIN, *STDOUT, 1);
}

exit 0;
