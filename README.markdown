
CTagSpotlight
=============

A binding of [Exuberant CTags](http://ctags.sourceforge.net/) for spotlight.

With this spotlight module, Spotlight will be able to index top level structures
of you code such as structures, functions, classes and methods. Then when searching
a project or any kind of code, you will be able to refine your search by specifying
what kind of element you went to be in the search.

Installation
------------
To install the mdimporter, you just have to drop it in
the `/Library/spotlight` folder.

TODO : add directive to update code files tags

Supported attributes
--------------------
When searching in spotlight, you can add the following criteria to
narrow your search :

 * Source code Functions
 * Source code Classes
 * Source code Macros
 * Source code Methods
 * Source code Constructors
 * Source code Types
 * Source code Module
 * Source code Variable
 * Source code Structure
 * Source code Constant
 * Source code Enum

To be able to use it, you must start a spotlight search, click
on the (+) button to add a search criteria, and find them in
the "Other" categories (they are not in the list directly).

Supported languages
-------------------
The mdimport support the following system defined language :

 * Assembler
 * C
 * C++
 * Java
 * JavaScript
 * Objective C
 * PHP
 * Perl
 * Python
 * Ruby
 * Sh

Additionally, the module support the following languages :

 * C#
 * Eiffel
 * Erlang
 * Fortran
 * Lisp
 * Lua
 * OCaml
 * Pascal
 * SML
 * Scheme
 * TCL
 * VHDL
 * Verilog

Build
-----
Open the xcode project, build, done.

Licence
-------
The same as [Exuberant CTags](http://ctags.sourceforge.net/) so it's
[GPL](http://www.gnu.org/copyleft/gpl.html) I guess. CTagSpotlight
integrate a tweaked version of CTags up to version 5.8 with patches
from the SVN repository.

