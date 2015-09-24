#include "indexhandler.h"
#include <algorithm>
#include <fstream>
#include <cstdlib>

#define MAX_PRINT_PAGE 10

/*
	This file contains functions required to launch search.
	It provides command line interface to user to do multiple
	searches. For each search limited number of search results
	are shown and user can choose to see more results.
*/

int min(int a, int b){
	if(a <= b) return a;
	else return b;
}

//Print pnum search results from v
int  print_num(docweight_vector v, int start, int pnum, docid_map* docmap){
	int i = 0;

	//Get absolute path of execution
	//Used to give file links in the terminal
	char szTmp[100];
	char pBuf[100];
	int len = 100;
	sprintf(szTmp, "/proc/%d/exe", getpid());
	int bytes = min(readlink(szTmp, pBuf, len), len - 1);
	if(bytes >= 0) pBuf[bytes-6] = '\0';

	//Print results
	cout << (start + 1) << " to " << (start + pnum) << endl;
	for(i=0; i < pnum; i++){
		//fetch document path from seek_data to give the link with "fetchdocid()"
		cout<< start + i +1<< " : " << v[start + i].second << ": " << "file://" << pBuf << fetchdocid(v[start + i].first, docmap) << endl;
	}

}

//Print search results in vector v
//Option for a new search or more search results
void print_results(docweight_vector v, docid_map* docmap){
	int num_result = v.size();	//number of search results

	int remaining = num_result;	//results remaining to be printed
	int start = 0;
	int flag = -1;

	cout << "Retrieved " << num_result << " documents"<<endl;

	int p_num ;
	if(num_result==0){
		cout << "No more relevant documents" << endl;
		return;
	}
	
	while(1){
		
		flag = -1;
		p_num = min(MAX_PRINT_PAGE, remaining);	//number of docs to print

		//print p_num results
		print_num(v, start, p_num, docmap);

		//update vars
		remaining -= p_num;
		start += p_num;

		//give option to print more results or new search
		//M: Print more results
		//S: Start new search
		if(remaining != 0){
			cout << "Type \'M\' to  print next " << min(remaining,MAX_PRINT_PAGE) << " documents" << endl;
			cout << "Type \'S\' to start a new search"<< endl;
		}
		else{
			flag = 1;
			cout << endl;
			cout << "All results printed" << endl;			
		}

		char inp;
		scanf("%c",&inp);
		getchar();

		switch(inp){

			case 'M':
				flag = 0;
				break;
			case 'm':
				flag = 0;
				break;
			case 'S':
				flag = 1;
				break;
			case 's':
				flag = 1;
				break;
			default:
				break;			
		}

		if(flag == 0) continue;
		if(flag == 1) break;
	}
}

//Call python parsers and nl-processors to parse and normalize query
string normalizeinputquery(string query)
{
	FILE* file;
	int argc;
	char* argv[2];

	//query parser python code path
	argv[0] = (char*) malloc(strlen(QUERY_PARSER)*sizeof(char));
	argv[1] = (char*) malloc(query.size()*sizeof(char));

	argc = 2;
	strcpy(argv[0], QUERY_PARSER);
	strcpy(argv[1], query.c_str());

	//Call python parsers and nl-processors
	//output will be written to a file
	Py_SetProgramName(argv[0]);
    Py_Initialize();
    PySys_SetArgv(argc, argv);
    file = fopen(QUERY_PARSER, "r");
    PyRun_SimpleFile(file, QUERY_PARSER);
    Py_Finalize();

    //read parsed query from output file of above code
    //and return the same
    fclose(file);
    ifstream query_parsed_file;
    query_parsed_file.open(QUERY_PARSE_PATH);
    string parsed_query;
    getline(query_parsed_file, parsed_query);
    query_parsed_file.close();
    return parsed_query;
}

int main()
{
	//load persistent vectors in memory
	fixed_name_map* termmap = loadtermmap();
	docid_map* docmap = loaddocidmap();
	inverted_index_map* invindex = loadinvertedindex();
	docweight_map* docweight = loaddocweightmap();

	//initialize static vars
	NUM_DOCS = docmap->size();
	NUM_TERMS = termmap->size();

	string query;
	while(1)
	{
		//get search query
		cout<<"Search Query:\t\n"<<endl;
		getline(cin, query);

		//parse query with python parsers and nl-prosessors in the package
		string parsed_query = normalizeinputquery(query);

		//process the query to get search results as ranked list of docs with scores
		docweight_vector results = query_processor(parsed_query, termmap, invindex, docweight);

		//Print the search results
		print_results(results, docmap);
	}
	return 0;
}