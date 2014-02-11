**jzmq - Jonathan's ZeroMQ Utility Library (C++)**
---------
---------

**Overview**
--------

This library is a simple wrapper around the amazing ZeroMQ library (http://zeromq.org/).  It does not add any extra features, but was created as an abstraction layer for some of my research projects.

**Compilation**
---------------

Building jzeromq uses Visual Studio 2012 on Windows, and cmake + gcc 4.7 (or greater) on Mac OS X.  The only real dependancy is the jtil library.  See <http://github.com/jonathantompson/jtil> for more details.

VS2012 and cmake expect a specific directory structure:

- \\include\\WIN\\
- \\include\\MAC\_OS\_X\\
- \\lib\\WIN\\
- \\lib\\MAC\_OS\_X\\
- \\jtil\\
- \\jzeromq\\

So the dependancy headers and static libraries (.lib on Windows and .a on Mac OS X) are separated by OS and exist in directories at the same level as jtorch, jtil and jcl.  I have pre-compiled the dependencies and put them in dropbox, let me know if you need the link.

**Style**
---------

This project follows the Google C++ style conventions: 

<http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml>
