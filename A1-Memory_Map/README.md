CMPUT 397 Assignment 1 - Stephen Just
=====================================

This program scans the memory layout of the machine it is
running on.

The program will loop over every page available in its 4GB
memory space and determine if it is accesible by the program, 
and if accessible, whether it is writable.

The program uses knowledge of the two kinds of segfaults that 
are possible, SEGV_MAPERR and SEGV_ACCERR. In the case of 
MAPERR, it means that the program has no read or write access to 
the page of memory. In the case of ACCERR, the memory is mapped 
to the program but is not writable, such as with the memory 
pages containing linked libraries.
