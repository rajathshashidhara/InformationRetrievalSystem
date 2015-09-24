#include "indexhandler.h"
#include <Python.h>
#include <iostream>
#include <cstdlib>
using namespace std;

/*
    This is file contains function to setup the index for the package.
    INPUT: Path of corpus to be indexed.
    OUTPUT: indexed persistent files of the corpus merges with existing
            index in the package. HTML and TXT files moved to respective
            directories of the package in data.
            Also TXT version of term-termid, doc-docid and posting-list
            merged with existing data is created in temp.
*/

void indexfiles(const char* path)
{
    remove(PERSISTENT_TERM_MAP);
    remove(PERSISTENT_DOC_MAP);
    remove(PERSISTENT_POS_LIST);
    remove(PERSISTENT_DOCWEIGHT_MAP);
	FILE* file;
	int argc;
	char* argv[2];

	argc = 2;
    argv[0] = (char*) malloc(strlen(PY_INDEXER)*sizeof(char));  //python exec path for indexing
    argv[1] = (char*) malloc(strlen(path)*sizeof(char));
	strcpy(argv[0], PY_INDEXER);
	strcpy(argv[1], path);

    //Call python functions to parse and process the docs in the path
    //calls the "pyindexer.py" file in bin/pyexec
    //creates termmap.txt, docmap.txt, plist.txt in temp
	Py_SetProgramName(argv[0]);
    Py_Initialize();
    PySys_SetArgv(argc, argv);
    file = fopen(PY_INDEXER, "r");
    PyRun_SimpleFile(file, PY_INDEXER);
    Py_Finalize();

    //use termmap.txt, docmap.txt, plist.txt to create persistent vectors
    //generate termmap vector
    fixed_name_map* termmap = generatetermmap();

    //generate inverted index (posting-list) vector
    inverted_index_map* invindex = generateinvertedindex(termmap);

    //generate docmap vector
    docid_map* docmap = generatedocmap();
    NUM_DOCS = docmap->size();  //static var initialization

    //generate docweightmap vector
    generatedocweightmap(invindex);
    
    free(argv[0]);
    free(argv[1]);
}

int main(int argc, char** argv)
{
    indexfiles(argv[1]);    //argv[1] contains path of corpus to be indexed
}