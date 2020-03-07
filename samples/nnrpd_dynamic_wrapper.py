##  $Id: nnrpd_dynamic_wrapper.py 10282 2018-05-14 12:42:14Z iulius $
##
##  Example wrapper for support of old Python authentication scripts,
##  by Erik Klavon.
##
##  This file contains a sample Python script which can be used to
##  duplicate the behaviour of the old nnrppythonauth functionality.
##  This script only supports dynamic access control by group.
##
##  How to use this wrapper:
##    - insert your authentication class into this file;
##    - rename your authentication class OLDAUTH.
##
##  See the INN Python Filtering and Authentication Hooks documentation
##  for more information.
##  The use of this file is *discouraged*.

##  Old AUTH class.
##  Insert your old auth class here.
##  Do not include the code which sets the hook.




##  Wrapper DYNACCESS class.  It creates an instance of the old class and
##  calls its methods.  Arguments and return values are munged as
##  needed to fit the new way of doing things.

class MYDYNACCESS:
    """Provide dynamic access callbacks to nnrpd."""
    def dynamic_init(self):
        self.old = OLDAUTH()

    def dynamic(self, attributes):
        return (self.old).authorize(attributes)

    def dynamic_close(self):
        (self.old).close()


##  The rest is used to hook up the dynamic access module on nnrpd.
##  It is unlikely you will ever need to modify this.

##  Import functions exposed by nnrpd.  This import must succeed, or nothing
##  will work!
from nnrpd import *

##  Create a class instance.
mydynaccess = MYDYNACCESS()

##  ...and try to hook up on nnrpd.  This would make auth object methods visible
##  to nnrpd.
import sys
try:
    set_auth_hook(mydynaccess)
    syslog('notice', "dynamic access module successfully hooked into nnrpd")
except Exception: # Syntax valid in both Python 2.x and 3.x.
    e = sys.exc_info()[1]
    syslog('error', "Cannot obtain nnrpd hook for dynamic access method: %s" % e.args[0])
