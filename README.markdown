
CTagSpotlight
=============

A binding of [Exuberant CTags](http://ctags.sourceforge.net/) for spotlight.

With this spotlight module, Spotlight will be able to index top level structures
of you code such as structures, functions, classes and methods. Then when searching
a project or any kind of code, you will be able to refine your search by specifying
what kind of element you went to be in the search.

Installation
------------
To installe the mdimporter, you just have to drop it in
the `/Library/spotlight` folder.

TODO : add directive to update code files tags

Supported attributes
--------------------
When searching in spotlight, you can add the following critera to
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
on the (+) button to add a search critera, and find them in
the "Other" categories (they are not in the list directly).

Supported languages
-------------------
Right now the following languages are supported, more languages
will come as I complete the binding.

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

Build
-----
Open the xcode project, build, done.

