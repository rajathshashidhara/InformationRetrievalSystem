=====================
SYSTEM REQUIREMENTS
=====================
OS: linux
Dependencies: Python packages - nltk, BeautifulSoup4, Python-BloomFilter, unidecode
python-dev
build-essentials
git
cmake
C++ Packages - STXXL (installed automatically during installation); 
additionally all dependencies of STXXL are required - visit the STXXL website provided in the references.

=====================
INSTALLATION
=====================

Clone the repository in the directory in which you want to install the system

This will create a directory called "ir_sys"

Change directory to "ir_sys"
$ cd ir_sys 

Make the "install.sh" executable:

$ chmod +x install.sh

Run the install shell script

$ sh install.sh

================================================
BUILDING INDEX / ADDING NEW FILES TO THE CORPUS
=================================================

Traverse to the installation directory

$ cd ir_sys
where ir_sys is the installation path

Run the buildindex executable:

$ ./buildindex <path>

<path> is the the absolute path to the home directory of the corpus
            -OR-
<path> can also be the absolute path of a single file

For example, if you want to add the files "xyz.html" and "abc.txt" which are in /home/username/corp/ run:

$ ./buildindex /home/username/corp

(NOTE: DO NOT type a '/' at the end)

FOr single file:

$ ./buildindex /home/username/my_files/xyz.html

(FILE CAN BE EITHER HTML OR TXT)

==========
SEARCHING
==========
Run the "search" executable

$ ./search

Enter the search query when prompted.

============
 REFERENCES
============

[1] http://stxxl.sourceforge.net/
[2] https://github.com/jaybaird/python-bloomfilter
[3] http://www.nltk.org/
[4] http://www.crummy.com/software/BeautifulSoup/
[5] https://docs.python.org/2/extending/embedding.html
