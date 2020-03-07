##  $Id: nnrpd.py 8250 2008-12-23 12:00:46Z iulius $
##
##  This module supplies stub Python functions corresponding to the ones
##  provided by nnrpd.  It is not used by the server; it is only here so
##  that you can test your filter scripts before loading.
##  See the INN Python Filtering and Authentication Hooks documentation
##  for more information.

from types import *

def set_auth_hook(anObject):
    print("** set_auth_hook for " + repr(anObject))

def syslog(level, message):
    print("-- syslog level: %s message: %s" % (level, message))
