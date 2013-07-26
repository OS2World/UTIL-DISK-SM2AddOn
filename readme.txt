SM2ADDON Copyright (c) 1996-98 by Steffen Siebert (siebert@logware.de)
======================================================================


  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


Introduction:

The IBM Search Manager/2 is a very powerful tool for easy finding a
document which you remember being on your hard-drive, but have absolutely no
idea, where it is or how it's named.

Unfortunately the SM/2 is held secret by the IBM marketing and even
support is hard to get (don't bother to search the Internet for fixes,
just write to the support-staff on Compuserve, Forum IBMDES). But the
tool is rather cheap and the demo looked very well to me, so I
decided to buy and use it.

Soon I've found that SM/2 lacks some essential features, making the
program rather useless to me: It can't handle compressed files, the
language of ASCII-files can only specified by filename-extension and
it can't even handle html-files.

These functions are rather simple to implement, if you've gotten the
source of the supplied dll's, which SM/2 uses for the OS/2
file-system. But unfortunately IBM didn't gave me the source (yes, I've
asked them), thus I had to reimplement the existing functions. But
after lots of programming and debugging, I finally got a working
replacement for the original dll's and could start writing the new
functions.

If you replace the original dll's with mine, you should see no
difference, unless you have gzipped file (with extension .gz) or
RMAIL files (with extension .rmail), which are always handled
by the AddOn DLL's.

The other new functions only apply, if you enable them by setting
environment-variables.


1. RMAIL Support *new*
================

SM/2 AddOn now supports RMAIL files (aka Babyl Format) used by GNU Emacs
to store mails and news.

SM/2 AddOn treats files with extension .rmail like a directory containing
files, where each file correspond to one mail. The name of the virtual
directory is the same as the rmail file, the virtual files are named
after the subject line of the mail they contain.

SM/2 AddOn creates a special index file with extension .rmail.rdx
for each RMAIL file to speed up the access to single mails. You can
delete these index files without doing much harm, since SM/2 will
recreate them the next time it indexes the RMAIL file, it just slows
the indexing process down. (Note: if you use SM/2 on several computers
(e.g. at home and at work), you should not copy the index files created
on one system onto another or SM/2 might not index all messages on the
second computer!)

If you store mails in several languages into one RMAIL file, SM/2 AddOn
can use a heuristic to determine the language of each mail. Currently
only English and german mails are recognized. If you want this feature,
install the DLL's from the subdirectory "dll.ger" (see 6. Installation
for details). Please note that the creation of the .rmail.rdx file
is much slower when the heuristic is used.

For very short mails the heuristic can not determine the language,
the default language is used in this case.


2. html-Support
===============

For SM/2 to recognize HTML documents, you need an HTML to ASCII filter
program. There are several programs available, which can do this job.
The program should only expect two parameters, first the name of the
HTML-file, second the name of the ASCII output file. I'm using the UNH
2.12 HTML stripper by Don Hawkinson.

The HTML support is enabled by assigning the name of the filter to the
environment variable SM2_HTML_FILTER. SM/2 then treats files with the
extensions ".htm", ".html" and ".shtml" as HTML files.


3. gzip-Support
===============

If you have a large collection of text-files on your hard-disk, you
probably want to store them compressed to save disk-space. The SM/2
AddOn DLL's are linked to the zlib library, thus they can handle
gzipped files without using external tools (like gzip).

SM/2 will decompress all files with the extension .gz and treat them
as if they were stored uncompressed on the disk, using the old
filename minus the .gz extension as the new filename.


4. Ignore files
===============

Sometimes you have a directory with text-files, which SM2 should index,
but there are also some files, which SM2 should ignore: for example the
graphic files among the HTML-Files you've downloaded from the
www, or the backup files which your favourite editor automaticly
creates.  Just set the environment-variable SM2_IGNORE_EXT to a
semicolon delimited list of file-extensions (including the dot, if it's
a "real" extension), and SM/2 will skip any file with a matching
extension.

Example: add "SET SM2_IGNORE_EXT=.gif;.jpg;~" to your config.sys and
SM2 will ignore your GIFs and JPEGs as well as any emacs-backupfile,
which ends with a tilde (note the missing dot!).


5. Set file attributes per directory
====================================

If you have a "stupid" text-format like ASCII, the only way to tell
SM2 something about the content (like the language) was to define a
special file-extension (no more than 3 letters!) and rename the file.

But now you can write a special resource-file, set the name to the
environment-variable SM2_RC_FILENAME, and SM2 will search the
directories with files to index for this file and treat any text-file
in the same directory according to the rules defined in the
resource-file.

If you add "SET SM2_RC_FILENAME=sm2info.rc" to your config.sys and
write a file named sm2info.rc, which contains the following 3 lines:

* e ascii
deutsch.txt d ascii
lies.mich d word

then SM2 will handle deutsch.txt as german ASCII-File, lies.mich as
german Word-File and every other file as an english ASCII-File.
(Note: SM2 seems to ignore the supplied file-type and determines it by
the file-contents, so even if you declare all files to be ASCII, SM2
will handle files created by word processors correct. I think of it as
a bug, but IBM tells me it's a feature :)

The format of the resource file is the following:

[<filename>|*] [d|e] [ascii|swriter|word]

the filename must not contain any wildcards

d is for german language ("deutsch") e is for english language

ascii is for ASCII-Files swriter is for Starwriter Files word is for
Word Files

If you need support for other languages or want to specify the
codepage, mail me.

(You may use any other filename for your resource-files, if you set it to
SM2_RC_FILENAME. But choose a name which is unlikely to match a "real"
document, since SM2 will never index a file with this name)


6. Installation
===============

First of all, you should have successfully installed SearchManager/2!

Create an empty directory on your harddisk and copy the files ehslscfs.dll
and ehslssfs.dll into it. This release contains two versions of these
dll's, please read section 1 - RMAIL Support, for details on the version
in subdirectory "dll.ger".

Unless you use RMAIL files with german _and_ english emails, you should
install the dll's stored in the subdirectory "dll".

Add the name of the directory to the LIBPATH statement of the config.sys
and make sure it is in front of the dll-path of SM/2 (x:\sm2\dll).

Enable some or all features of the SM2ADDON by adding the environment
variables described before to the config.sys

Reboot and run SearchManager/2.


7. Deinstallation
=================

If you want to remove SM2ADDON, just undo the changes made to the config.sys
during installation, reboot and delete the dll's.


8. Source *new*
=========

This release includes the complete source.

Please read "The Cathedral and the Bazaar" 
(http://sagan.earthspace.net/~esr/writings/cathedral-bazaar/)
if you want to know my motivation to release it.

The source is based on the sample code provided by IBM with the SM/2
Toolkit.

Currently you need the IBM Visual Age C++ 3.0 compiler to compile the
source.

It also compiles with the free EMX 0.9c GCC compiler, but the resulting
DLL's are not working. (If someone manages to build them successfully
with EMX, please send me the patches.)

The DLL's are using two libraries, whose source is also included:
zlib 1.1.1 and GNU rx 1.5.

To create the DLL's, unzip the file src.zip, go to the src subdirectory
and type "make".

If there is any problem, please read the makefile for further information.


9. Support
==========

Please read the file sm2addon.faq, which answers several frequently
asked question.

SM2ADDON comes without any warranty or support, but if you have problems,
questions or any comments about SM2ADDON, feel free to send me an email
to the following address:

siebert@logware.de

For further informations on SM2ADDON, please visit it's homepage at

http://www.cs.tu-berlin.de/~siebert/sm2.html

You can register the page to receive email on new releases.

