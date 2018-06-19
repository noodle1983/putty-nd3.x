#!/usr/local/bin/perl

my $version_no = `grep Noodle ../putty/version.h`;
chomp $version_no;
$version_no =~ s/^.*?([0-9\.]+).*$/\1/;
$postfix = '';

print `rm "./release/putty_nd${version_no}${postfix}.zip" ./putty.exe`;
print `cp ../bin/chrome.exe ./putty.exe`;
print `./upx.exe ./putty.exe`;
print `./zip -9 -v "./release/putty_nd${version_no}${postfix}.zip" putty.exe release_note.md`;
print `cp release_note.md ./release/readme.md`;
