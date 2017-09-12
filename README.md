tclmpg123
=====

[mpg123](http://mpg123.org/) is a free and open-source audio player.
libmpg123 is mpg123's decoding library.

This extension is Tcl bindings for libmpg123.


License
=====

LGPL 2.1, or (at your option) any later version.


Commands
=====

mpg123 HANDLE path ?-buffersize size?  
HANDLE buffersize size  
HANDLE read   
HANDLE seek location whence  
HANDLE get_ID3v1  
HANDLE close

seek command option `whence` have 3 values, SET, CUR and END.


UNIX BUILD
=====

I only test tclmpg123 under openSUSE LEAP 42.2 and Ubuntu 14.04.

Users need install libmpg123 development files.
Below is an example for openSUSE
(maybe need via [Packman](https://en.opensuse.org/Additional_package_repositories#Packman)):

	sudo zypper in libmpg123-devel

If you use openSUSE LEAP 42.3, you can try below command to install development files:

	sudo zypper in mpg123-devel

Below is an example for Ubuntu:

	sudo apt-get install libmpg123-dev

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following minimal example
will install the extension in the /opt/tcl directory.

	$ cd tclmpg123
	$ ./configure --prefix=/opt/tcl
	$ make
	$ make install

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

	$ cd tclmpg123
	$ ./configure --with-tcl=/opt/activetcl/lib
	$ make
	$ make install

WINDOWS BUILD
=====

## MSYS2/MinGW-W64

Download mpg123 source code and build.

	$ ./configure --prefix=/c/msys64/mingw64
	$ make
	$ make install

Put libmpg123-0.dll to Windows folder or other available folder.

Next step is to build tclmpg123.

	$ ./configure --with-tcl=/c/tcl/lib
	$ make
	$ make install

Example
=====

Cowork with [tcllibao](https://github.com/ray2501/tcllibao).

	#
	# Using libao and libmpg123 to play a mp3 file
	#

	package require libao
	package require mpg123

	if {$argc > 0} {
	  set name [lindex $argv 0]
	} else {
	  puts "Please input filename."
	  exit
	}

	if {[catch {set data [mpg123 mpg0 $name]}]} {
	  puts "mpg123: read file failed."
	  exit
	}
	set bits [dict get $data bits]

	# only for test seek function
	mpg0 seek 0 SET

	libao::ao initialize
	set id [libao::ao default_id]

	libao::ao open_live $id -bits $bits \
	  -rate [dict get $data samplerate] \
	  -channels [dict get $data channels]

	while {[catch {set buffer [mpg0 read]}] == 0} {
	  libao::ao play $buffer
	}

	mpg0 close
	libao::ao close
	libao::ao shutdown

