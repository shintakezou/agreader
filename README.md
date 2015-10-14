# AGReader

This project leverages AGReader (agr) by Thierry Pierron (and
Hans de Goede and [Ian Chapman](http://amiga-hardware.com/)) in
order to provide a more modern experience with the reading
of AmigaGuide files.

The text of the original README, slightly edited and markdownized,
follows.


## AGReader (agr) (original text)

A console based utility for reading AmigaGuide files. It supports all
of the v39 specification possible and a large subset of the v40
specification.

AGReader was written by *Thierry Pierron* up to V1.1. Version 1.2
incorporates patches created by *Hans de Goede* and applied by
*Ian Chapman*. These patches fix compilation issues with GCC 4.1 and
fix compatibility issues with the Home, End, F1, F2 and F3 keys under
linux.

AGReader V1.2 has only been tested on linux (x86, ppc, x86_64) but may
compile under other UNIX and UNIX-like operating systems with varying
degrees of compatibility.

AGReader is no longer being actively maintained. If you do have any
problems with V1.2, please report it to packages@amiga-hardware.com
and if possible, include a patch, however there is no guarantee a fix
will be made, or applied.


## Installation (original text)

These are generic compilation and installation instructions for most
UNIX type operating systems.

    cd Sources
    make
    make install
