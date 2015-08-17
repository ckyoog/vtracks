#!/usr/bin/env perl

#yum install perl-Email-Simple

use 5.010;
use strict;
use warnings;
use Email::Simple;
use Path::Class;
use File::Temp;

use File::Basename;
use Cwd qw(abs_path);
 
die "$0: This is not an svn repo\n"
  unless -d '.svn';
 

	my %userpass = ("Yongshen Liu", "123456",
		"Yan Zhu", "abcdef");

for my $f (sort @ARGV) {
  say "*** $f ***";
  # try to apply patch
  #system("patch -p1 < $f");
  system("git apply --verbose --whitespace=nowarn $f");
  die "Patch failed!  Aborting!\n" if $?;
 
  # extract commit info
  my $email = Email::Simple->new( scalar file($f)->slurp );
  my $subject = $email->header("Subject");
  $subject =~ s{^\[PATCH[^\]]*\]\s+}{};
  my $body = $email->body;
  $body =~ s{\A(.*?)---.*$}{$1}ms;
	my $username = $email->header("From");
	$username =~ s{ *<[a-zA-Z]+@[^>]*> *$}{};
	#say "username: " . '"' . $username . '"';
	my $password = defined($userpass{$username}) ? $userpass{$username} : "123456";
	#say "password: " . '"' . $password . '"';
	$username = 'ykou';
	$password = 'password';
 
  # write commit info to tempfile and commit it
  my $temp = File::Temp->new;
  say {$temp} $subject;
  print {$temp} "\n$body" if $body;
 
  #system("svn commit -F $temp");
	system(dirname(abs_path($0)) . "/svn-commit.sh --username \"$username\" --password \"$password\" -F $temp");
  die "Commit failed!  Aborting!\n" if $?;
	#system("svn up");
	#die "Update failed!  Aborting!\n" if $?;
}
