#!/usr/local/bin/perl

my $version_no = `grep Noodle ../putty/version.c`;
chomp $version_no;
$version_no =~ s/^.*?([0-9\.]+).*$/\1/;

print `rm "./release/putty_nd$version_no.zip" ./putty.exe`;
print `cp ../bin/chrome-d.exe ./putty.exe`;
print `./upx.exe ./putty.exe`;
print `./zip -9 -v "./release/putty_nd$version_no.zip" putty.exe release_note.txt`;
print `cp release_note.txt ./release/readme.txt`;
