##  $Id: INN.py 8250 2008-12-23 12:00:46Z iulius $
##
##  This module supplies stub Python functions corresponding to the ones
##  provided by innd.  It is not used by the server; it is only here so
##  that you can test your filter scripts before loading.
##  See the INN Python Filtering and Authentication Hooks documentation
##  for more information.

from types import *

def set_filter_hook(anObject):
    print("** set_filter_hook for " + repr(anObject))

def addhist(messageid):
    print("** addhist Message-ID: " + messageid)

def havehist(messageid):
    print("** havehist Message-ID: " + messageid)

def cancel(messageid):
    print("** cancel Message-ID: " + messageid)

def newsgroup(groupname):
    print("** newsgroup: " + groupname)

def head(messageid):
    print("** head Message-ID: " + messageid)

def article(messageid):
    print("** article Message-ID: " + messageid)

def hashstring(mystring):
    print("** hash: " + mystring)

def syslog(level, message):
    print("-- syslog level: %s message: %s" % (level, message))
