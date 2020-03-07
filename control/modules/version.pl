##  $Id: version.pl 8281 2009-01-10 11:52:16Z iulius $
##
##  version control message handler.
##
##  Copyright 2001 by Marco d'Itri <md@linux.it>
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions
##  are met:
##
##   1. Redistributions of source code must retain the above copyright
##      notice, this list of conditions and the following disclaimer.
##
##   2. Redistributions in binary form must reproduce the above copyright
##      notice, this list of conditions and the following disclaimer in the
##      documentation and/or other materials provided with the distribution.

use strict;

sub control_version {
    my ($par, $sender, $replyto, $site, $action, $log, $approved,
        $article) = @_;
    my ($where) = @$par;

    my $head = $article->head;
    my @headers = split(/\r?\n/, $head->stringify);
    my @body = split(/\r?\n/, $article->stringify_body);

    my $version = $INN::Config::version || '(unknown version)';

    if ($action eq 'mail') {
        my $mail = sendmail("version $sender");
        print $mail <<END;
$sender has requested information about your
news software version.

If this is acceptable, type:
  echo "InterNetNews $version" | $INN::Config::mailcmd -s "version reply from $INN::Config::pathhost" $replyto

The control message follows:

END
        print $mail map { s/^~/~~/; "$_\n" } @headers;
        print $mail "\n";
        print $mail map { s/^~/~~/; "$_\n" } @body;
        close $mail or logdie("Cannot send mail: $!");
    } elsif ($action eq 'log') {
        if ($log) {
            logger($log, "version $sender", $article);
        } else {
            logmsg("version $sender");
        }
    } elsif ($action =~ /^(doit|doifarg)$/) {
        if ($action eq 'doifarg' and $where ne $INN::Config::pathhost) {
            logmsg("skipped version $sender");
            return;
        }
        sendmail("version reply from $INN::Config::pathhost", $replyto,
            [ "InterNetNews $version\n" ]);

        logger($log, "version $sender to $replyto", $article) if $log;
    }
}

1;
