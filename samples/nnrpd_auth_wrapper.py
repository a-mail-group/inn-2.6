##  $Id: nnrpd_auth_wrapper.py 10282 2018-05-14 12:42:14Z iulius $
##
##  Example wrapper for support of old Python authentication scripts,
##  by Erik Klavon.
##
##  This file contains a sample Python script which can be used to
##  duplicate the behaviour of the old nnrppythonauth functionality.
##  This script only supports authentication.
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




##  Wrapper AUTH class.  It creates an instance of the old class and
##  calls its methods.  Arguments and return values are munged as
##  needed to fit the new way of doing things.

class MYAUTH:
    """Provide auth callbacks to nnrpd."""
    def authen_init(self):
        self.old = OLDAUTH()

    def authenticate(self, attributes):
        # Python 3.x uses memoryview(b'authinfo') because buffers
        # do not exist any longer.  Note that the argument is
        # a bytes object.
        #  attributes['type'] = memoryview(b'authinfo')
        #  perm = (self.old).authenticate(attributes)
        # whereas in Python 2.x:
        #  attributes['type'] = buffer('authinfo')
        #  perm = (self.old).authenticate(attributes)
        response = 281
        err_str = "No error"
        #  if perm[0] == 481:
        #      response = perm[0]
        #      err_str = "Python authentication error!"
        return (response, err_str)

    def authen_close(self):
        (self.old).close()


##  The rest is used to hook up the auth module on nnrpd.  It is unlikely
##  you will ever need to modify this.

##  Import functions exposed by nnrpd.  This import must succeed, or nothing
##  will work!
from nnrpd import *

##  Create a class instance.
myauth = MYAUTH()

##  ...and try to hook up on nnrpd.  This would make auth object methods visible
##  to nnrpd.
import sys
try:
    set_auth_hook(myauth)
    syslog('notice', "authentication module successfully hooked into nnrpd")
except Exception: # Syntax valid in both Python 2.x and 3.x.
    e = sys.exc_info()[1]
    syslog('error', "Cannot obtain nnrpd hook for authentication method: %s" % e.args[0])
