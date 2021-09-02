# Snakify
This tiny command-line application turns all tokens it can find that are in ``camelCase`` form into ``snake_case`` form. Can be built from a single C++20 source file. ``PascalCase`` tokens remain unconverted. It will recursively iterate through all desired files in a directory, including subdirectories, and write the converted code into an output directory with all files in the same relative order as the original input directory structure, including all subdirectories.

Here are all command-line options:
* ``-extensions=``: Files with the extensions you specify here will be converted by Snakify, all other files will be ignored and remain untouched. Example: ``-extensions=cc,cpp,hh,hpp``
* ``-inputdir=``: Specifies the directory that contains all files that you want converted. You can specify an absolute path or a path relative to the Snakify executable. Example: ``-inputdir=../../MyProject``
* ``-outputdir=``: Specifies the directory that will contain all transformed files. The relative subdirectory locations of all input files will be replicated in the output directory. You can specify an absolute path or a path relative to the Snakify executable. Example: ``-outputdir=../../MyProjectConverted``
