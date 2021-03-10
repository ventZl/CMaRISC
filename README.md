C-MARISC
========

This is an old attempt to create very minimalistic virtual machine
and a compiler for it. The nickname of the virtual machine was 
C Minimal Architecture Reduced Instruction Set Computer. The aim was
to create an architecture with minimal amount of instructions, for
which you can write a C compiler. This code also somehow predates
widespread use of LLVM/clang. Today I would probably not bother with
writing a compiler from scratch. But it was an interesting experience.

About MaRISC
============

MaRISC was an experimental architecture. It was designed to be somewhere
in between ARM and Thumb. It is a 16bit Von Neumann processing unit.
It's instruction set is composed of 16bit fixed length instructions. Main
intent was to run this virtual machine on top of AVR MCU. The goal was to
design the instruction set which allows rather simple implementation of
C compiler. Execution speed wasn't probably among the best.

Content
=======

libvm
-----

This is single function virtual machine implementing MaRISC ISA. You can 
link it to your code and provide two functions for reading and writing data
from / to memory. It was possible to compile this on AVR and not eat up all 
the flash.

libcmdline
----------
Hungry programmer's implementation of commandline argument parser. Roughly
similar to Python's argparse.

libobject
---------
Naiive implementation of object files.

mar
---
Minimal archiver. This tool created archives composed of object files 
produced by compilation of individual C files.

mas
---
Minimal (non-optimizing) assembler. This tool converts assembly files into
MaRISC bytecode. It roughly follows Turbo Assembler syntax convention.

mcc
---
Minimal (not-optimizing-a-lot) compiler. This is compiler for preprocessed 
C code. It outputs MaRISC assembly. IIRC, it is not finished as it doesn't
contain working implementation for `for` and possibly `while` cycles. It
should be trivial to add it.

mdbg
----
Minimal debugger. Wraps libvm into gdb-like user interface. It is rather bare
as it only allows to step and disassemble binary.

ml
--
Minimal linker. Takes a bunch of object files and/or archives and converts it
to linked raw binary which can be loaded into memory. Only performs static
linking.

mpp
---
Minimal preprocessor. This is rather barebone and definitely not compliant
implementation of C preprocessor. IIRC it only provides support for file
inclusion, macro definition and undefinition and some conditionals based on
macro presence and maybe even value. This turned out to be the most
complicated part of code to be written. It is also not finished.

Note
====
You can feel, that the code is old, because comments inside the code are written
in my mother tongue rather than in english. I don't make this kind of mistake
these days :)