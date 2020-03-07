# 
# RCSId:        $Id: startup_innd.pl 42 1997-08-04 04:03:43Z gpalmer $
# Description:	Sample startup code for Perl hooks in INN. This file, after
#		it's installed in the right spot, will be loaded when
#		innd starts up. The following functions should be defined
#		by it (they don't have to be, in fact this file can be
#		empty, but it must exist if you've compiled in Perl support).
#
#		sub filter_before_reload { ... }
#			Called before the filter definition file filter_innd.pl
#			is loaded (every time).
#		sub filter_after_reload { ... }
#			Called after the filter definition file filter_innd.pl
#			is loaded (every time).
#
#		See the sample file filter_innd.pl for details on what it does.


my $before_count = 1 ;
# Gets no arguments, and its caller expects no return value.
sub filter_before_reload {
	if ($before_count == 1) {
#		Do one thing
#		print "First time (before)\n" ;
		$before_count++ ;
	} else {
#		Do something else
#		print "Time number $before_count (before)\n" ;
		$before_count++ ;
	}
}

my $after_count = 1 ;
# Gets no arguments, and its caller expects no return value.
sub filter_after_reload {
	if ($after_count == 1) {
#		Do one thing
#		print "First time (after)\n" ;
		$after_count++ ;
	} else {
#		Do another
#		print "Time number $after_count (after)\n" ;
		$after_count++ ;
	}
}

