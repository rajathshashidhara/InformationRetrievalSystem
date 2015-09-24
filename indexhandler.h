#include <Python.h>
#include <stxxl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <cmath>
#include <vector>

/* PATHS */
static const char* CORPUS = "data/";
static const char* EXEC   = "bin/";
static const char* TEMP   = "temp/";
static const char* PDATA  = "prog_data/";
static const char* VECT   = "prog_data/persistent/";
static const char* PERSISTENT_TERM_MAP = "prog_data/persistent/term_map.dat";
static const char* PERSISTENT_DOC_MAP = "prog_data/persistent/doc_map.dat";
static const char* PERSISTENT_POS_LIST = "prog_data/persistent/inv_index.dat";
static const char* PERSISTENT_DOCWEIGHT_MAP = "prog_data/persistent/docweight_map.dat";
static const char* SEEK = "prog_data/seek_data/";
static const char* SEEK_DOCMAP = "prog_data/seek_data/doc_map.bin";
static const char* SEEK_INVINDEX = "prog_data/seek_data/inv_index.bin";
static const char* SEEK_DOCWEIGHTMAP = "prog_data/seek_data/docweight_map.bin";
static const char* PYTHON_EXEC = "bin/pyexec/";
static const char* WIKI_PARSER = "bin/pyexec/wiki_parser.py";
static const char* QUERY_PARSER = "bin/pyexec/py_query_parser.py";
static const char* NL_PROC = "bin/pyexec/nl_proc.py";
static const char* PY_INDEXER = "bin/pyexec/pyindexer.py";
static const char* INDEXER = "bin/indexer";
static const char* TXT_CORPUS = "data/txt";
static const char* HTML_CORPUS = "data/html";
static const char* TEMP_TERMMAP = "temp/termmap.txt";
static const char* TEMP_DOCMAP = "temp/docmap.txt";
static const char* TEMP_POSTLIST = "temp/plist.txt";
static const char* QUERY_PARSE_PATH = "temp/parsed_query.txt";


/* CONSTANTS */
static const int MAX_KEY_LEN = 48;
static const int BLK_SIZE = 4096;
static const int FIXED_NAME_VEC_SIZE = BLK_SIZE*(MAX_KEY_LEN+sizeof(unsigned int));
static const int FIXED_NAME_VEC_PAR1 = 4;
static const int FIXED_NAME_VEC_PAR2 = 8;

/* static variables */
static int NUM_DOCS;
static int NUM_TERMS;

/* STRUCT DEFNS */
//STRUCT used to define STXXL Maps
struct CompareGreater
{
	bool operator () (const int & a, const int & b) const
	{
		return a>b;
	}

	static unsigned int max_value()
	{
		return std::numeric_limits<unsigned int>::min();
	}
};

//Class to be used in persistent map to store terms
class FixedString { 
public:
    char charStr[MAX_KEY_LEN];  //fixed length term (48 chars)

    //operator redefinitions
    // for <
    bool operator< (const FixedString& fixedString) const {
        return std::lexicographical_compare(charStr, charStr+MAX_KEY_LEN,
            fixedString.charStr, fixedString.charStr+MAX_KEY_LEN);
    }

    //for ==
    bool operator==(const FixedString& fixedString) const {
        return std::equal(charStr, charStr+MAX_KEY_LEN, fixedString.charStr);
    }

    //for !=
    bool operator!=(const FixedString& fixedString) const {
        return !std::equal(charStr, charStr+MAX_KEY_LEN, fixedString.charStr);
    } 
};

//STRUCT used to define STXXL Maps
struct comp_type : public std::less<FixedString> {
    static FixedString max_value()
    {
        FixedString s;
        std::fill(s.charStr, s.charStr+MAX_KEY_LEN, 0x7f);
        return s;
    } 
};

std::istream& operator >> (std::istream& i, FixedString& str)
{
    i >> str.charStr;
    return i;
}

std::ostream& operator << (std::ostream& o, FixedString& str)
{
    o << str.charStr;
    return o;
}

//Comparator for ranked list
struct ranking
{
    bool operator() (const std::pair <unsigned int, double> &a, const std::pair <unsigned int, double> &b) const
    {
        return a.second > b.second;
    }

    std::pair<unsigned int, double> min_value() const
    {
        return std::pair<unsigned int, double> (std::numeric_limits<unsigned int>::max(), std::numeric_limits<double>::max());
    }

    std::pair<unsigned int, double> max_value() const
    {
        return std::pair<unsigned int, double> (std::numeric_limits<unsigned int>::min(), 0.0);
    }
};

//STRUCT used to define STXXL Maps
struct docidcomparator
{
    bool operator() (const std::pair <unsigned int, double> &a, const std::pair <unsigned int, double> &b) const
    {
        return a.first < b.first;
    }

    std::pair<unsigned int, double> min_value() const
    {
        return std::pair<unsigned int, double> (std::numeric_limits<unsigned int>::min(), 0.0);
    }

    std::pair<unsigned int, double> max_value() const
    {
        return std::pair<unsigned int, double> (std::numeric_limits<unsigned int>::max(), std::numeric_limits<double>::max());
    }
};

/* TYPEDEFS */
typedef stxxl::map<FixedString, unsigned int, comp_type, BLK_SIZE, BLK_SIZE> fixed_name_map;
typedef stxxl::VECTOR_GENERATOR<std::pair<FixedString, unsigned int>, FIXED_NAME_VEC_PAR1, FIXED_NAME_VEC_PAR2, FIXED_NAME_VEC_SIZE>::result fixed_name_vector;
typedef fixed_name_map::iterator fixed_name_map_iterator;

typedef stxxl::map<unsigned int, long, CompareGreater, BLK_SIZE, BLK_SIZE> inverted_index_map;
typedef stxxl::VECTOR_GENERATOR<std::pair<unsigned int, long>, FIXED_NAME_VEC_PAR1, FIXED_NAME_VEC_PAR2, BLK_SIZE>::result inverted_index_vector;

typedef stxxl::map<unsigned int, unsigned int, CompareGreater, BLK_SIZE, BLK_SIZE> docid_map;
typedef stxxl::VECTOR_GENERATOR<std::pair<unsigned int, unsigned int> >::result docid_vector;

typedef stxxl::map<unsigned int, double, CompareGreater, BLK_SIZE, BLK_SIZE> docweight_map;
typedef stxxl::VECTOR_GENERATOR<std::pair<unsigned int, double> >::result docweight_vector;

using namespace std;

//Function to load persistent term map as vector into memory
fixed_name_map* loadtermmap()
{
    //persistent file
    stxxl::syscall_file* inputfile = new stxxl::syscall_file(PERSISTENT_TERM_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    
    //make vector out of file
    fixed_name_vector pVector(inputfile);
    fixed_name_map* termmap;

    termmap = new fixed_name_map((fixed_name_map::node_block_type::raw_size)*5, (fixed_name_map::leaf_block_type::raw_size)*5);
    termmap->insert(pVector.begin(), pVector.end());
    return termmap;
}

//Function to load persistent inverted index as vector into memory
inverted_index_map* loadinvertedindex()
{
    //persistent file
    stxxl::syscall_file* inputfile = new stxxl::syscall_file(PERSISTENT_POS_LIST, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    
    //make vector out of file
    inverted_index_vector pVector(inputfile);
    inverted_index_map* i_index;
    
    i_index = new inverted_index_map((inverted_index_map::node_block_type::raw_size)*5, (inverted_index_map::leaf_block_type::raw_size)*5);
    i_index->insert(pVector.begin(), pVector.end());    
    return i_index;
}

//Function to load persistent doc-id map vector into memory
docid_map* loaddocidmap()
{
    //persistent file
    stxxl::syscall_file* inputfile = new stxxl::syscall_file(PERSISTENT_DOC_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    
    //make vector out of file
    docid_vector pVector(inputfile);
    docid_map* docmap;
    
    docmap = new docid_map((docid_map::node_block_type::raw_size)*5, (docid_map::leaf_block_type::raw_size)*5);
    docmap->insert(pVector.begin(), pVector.end());
    return docmap;
}

//Extract and return posting list of termid
vector<pair<int, int> > fetchpostinglist(unsigned int termid, inverted_index_map* invindex)
{
    FILE* datafp = fopen(SEEK_INVINDEX, "rb");
    vector<pair<int, int> > v;

    //position of the posting list in file
    long seekindex = invindex->find(termid)->second;
    int buffer[1];
    fseek(datafp, seekindex, SEEK_SET);

    //read length from position into buffer
    fread(buffer, sizeof(int), 1, datafp);
    int* postlist = new int[(2*buffer[0])];

    //read posting list
    fread(postlist, sizeof(int), 2*buffer[0], datafp);

    //make vector from posting list
    for(int i =0; i<buffer[0]; i++)
    {
        v.push_back(pair<int,int> (postlist[2*i], postlist[2*i+1]));        
    }
    delete postlist;

    fclose(datafp);
    return v;
}

//Extract and return termid of given term
unsigned int fetchtermid(string term, fixed_name_map* termmap)
{
    FixedString fs;

    //strip term to 48 chars
    strncpy(fs.charStr, term.c_str(), MAX_KEY_LEN-1);

    //pad with ' '
    for(int i=term.length(); i<MAX_KEY_LEN-1; i++)
        fs.charStr[i] = ' ';
    fs.charStr[MAX_KEY_LEN-1] = '\0';

    return termmap->find(fs)->second;
}

//Extract and return document path of given docid
string fetchdocid(unsigned int docid, docid_map* docmap)
{
    //position of the path in file
    unsigned int seekindex = docmap->find(docid)->second;
    FILE* docfp = fopen(SEEK_DOCMAP, "rb");
    
    int buffer[1];
    fseek(docfp, seekindex, SEEK_SET);

    //read patn length
    fread(buffer, sizeof(int), 1, docfp);
    char* postlist = new char[(buffer[0])];

    //read path
    fread(postlist, sizeof(char), buffer[0], docfp);
    fclose(docfp);
    return string(postlist).substr(0,buffer[0]);    
}

//Generate persistent term-termid map
fixed_name_map* generatetermmap()
{
    stxxl::syscall_file* outputfile = new stxxl::syscall_file(PERSISTENT_TERM_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    fixed_name_vector pVector(outputfile);

    ifstream file;
    file.open(TEMP_TERMMAP);    //read from termmap.txt

    string line, term;  
    int termid, i;
    std::string::size_type sz;

    //get term termid pair from termmap.txt
    while(getline(file, line))
    {
        size_t spaceindex;

        spaceindex = line.find(' ');        
        
        term = line.substr(0, spaceindex);
        termid = stoi(line.substr(spaceindex+1), &sz);
        FixedString fs;
        strncpy(fs.charStr, term.c_str(), MAX_KEY_LEN-1);
        for(i=term.length(); i<MAX_KEY_LEN-1; i++)
            fs.charStr[i] = ' ';
        fs.charStr[MAX_KEY_LEN-1] = '\0';

        //add term-termid pair to vector
        pVector.push_back(pair<FixedString, unsigned int>(fs, termid));
    }
    
    //create map out of vector
    fixed_name_map* termmap;
    termmap = new fixed_name_map((fixed_name_map::node_block_type::raw_size)*5, (fixed_name_map::leaf_block_type::raw_size)*5);
    termmap->insert(pVector.begin(), pVector.end());
    return termmap;
}

//Generate persistent inverted index map
inverted_index_map* generateinvertedindex(fixed_name_map* termmap)
{
    stxxl::syscall_file* outputfile = new stxxl::syscall_file(PERSISTENT_POS_LIST, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    inverted_index_vector pVector(outputfile);

    long seeksize = 0;
    FILE* datafp = fopen(SEEK_INVINDEX, "wb+");

    ifstream inputfile;
    inputfile.open(TEMP_POSTLIST);  //read from plist.txt

    string line, term, subs;
    unsigned int termid;
    std::string::size_type sz;
    FixedString fs;
    int i, docfreq;
    std::stringstream ss;
    string s;
    int* buffer;
    int sepindex, sepindex2;

    //get post-list term wise
    while(getline(inputfile, line))
    {
        ss.str(std::string());
        ss.clear();
        seeksize = ftell(datafp);
        sepindex = line.find(' ');
        term = line.substr(0, sepindex);                //get term
        subs = line.substr(sepindex+1, string::npos);       
        
        //make fixed string by padding with spaces
        strncpy(fs.charStr, term.c_str(), MAX_KEY_LEN-1);
        for(i=term.length(); i<MAX_KEY_LEN-1; i++)
            fs.charStr[i] = ' ';
        fs.charStr[MAX_KEY_LEN-1] = '\0';

        //get termid
        termid = termmap->find(fs)->second;             

        i = 0;
                
        sepindex = subs.find(' ');
        term = subs.substr(0, sepindex);
        subs = subs.substr(sepindex+1, string::npos);
        ss<<term;
        ss>>docfreq;
        ss.str(std::string());
        ss.clear();

        //buffer contains docfreq and docid-termfreq pairs
        buffer = new int[(2*docfreq+1)];
        buffer[i] = docfreq;        
        i++;
        
        while(i != (2*docfreq+1))
        {
            if(i%2 == 1)
            {
                sepindex = subs.find(',');
                term = subs.substr(0, sepindex);
                subs = subs.substr(sepindex+1, string::npos);
            }
            else
            {
                sepindex = subs.find(' ');
                term = subs.substr(0, sepindex);
                subs = subs.substr(sepindex+1, string::npos);
            }
            ss<<term;
            ss>>buffer[i];
            ss.str(std::string());
            ss.clear();
            i++;
        }

        //write posting list to file
        fwrite(buffer, sizeof(int), 2*docfreq+1, datafp);

        //push termid and seek position to vector
        pVector.push_back(pair<unsigned int, long> (termid, seeksize));     
        delete buffer;
    }
    
    //make map out of vector
    inverted_index_map* i_index;
    i_index = new inverted_index_map((inverted_index_map::node_block_type::raw_size)*5, (inverted_index_map::leaf_block_type::raw_size)*5);
    i_index->insert(pVector.begin(), pVector.end());    

    fclose(datafp);
    return i_index;
}

//Generate persistent doc-docid map
docid_map* generatedocmap()
{
    stxxl::syscall_file* outputfile = new stxxl::syscall_file(PERSISTENT_DOC_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    docid_vector pVector(outputfile);

    ifstream file;
    file.open(TEMP_DOCMAP);                     //read docmap.txt
    
    FILE *ofile = fopen(SEEK_DOCMAP, "wb+");

    string line, path;  
    unsigned int docid,len,pos=0;
    std::string::size_type sz;
    
    //get docid doc-path pair
    while(getline(file, line))
    {
        size_t spaceindex;
        spaceindex = line.find(' ');        
        docid = atoi(line.substr(0, spaceindex).c_str());   
        path = line.substr(spaceindex+1);                                   //path of doc
        len = path.length();                                                //length of path
        pVector.push_back(pair<unsigned int, unsigned int>(docid, pos));    //push docid, position of path in file
        
        //write length and path to file
        fwrite(&len,sizeof(unsigned int),1,ofile);
        fwrite(path.c_str(),sizeof(char),len,ofile);

        //update postion
        pos += (len+sizeof(unsigned int));
    }   
    
    //make map out of vector
    docid_map* docmap;
    docmap = new docid_map((docid_map::node_block_type::raw_size)*5, (docid_map::leaf_block_type::raw_size)*5);
    docmap->insert(pVector.begin(), pVector.end());

    fclose(ofile);
    return docmap;
}

double calculate_tf_score(int termfreq)
{
    return termfreq;
}

double calculate_df_score(int docfreq)
{
    return log(((double)NUM_DOCS)/((double)docfreq));
}

//get length of doc with docid
double fetchdoclength(unsigned int docid, docweight_map* docweightmap)
{
    return docweightmap->find(docid)->second;
}

//process over all terms to calculate weight of each document w.r.t term
void term_processor(unsigned int term_id, docweight_vector& weight_map, inverted_index_map* invindex, docweight_map* docweightmap){
    int docfreq, termfreq;
    double weight;
    unsigned int docid;
    //Iterator for the weights map
    map<int,double>::iterator map_iter;

    //Create vector for posting list pf the given term
    vector< pair<int,int> > post_list = fetchpostinglist(term_id, invindex);
    //Iterator for the posting list vector
    vector< pair<int,int> >::iterator post_iter = post_list.begin();
    
    docfreq = post_list.size();
    while(post_iter != post_list.end()){ //Iterate throuh the postin list
        //get document id
        docid = post_iter->first;
        //get term frequency of the document
        termfreq = post_iter->second;
        
        //calculate the weight for the doument
        weight = calculate_tf_score(termfreq)*calculate_df_score(docfreq)*calculate_df_score(docfreq)/sqrt(fetchdoclength(docid, docweightmap));

        //map_iter = weight_map.find(docid);

        weight_map.push_back(pair<unsigned int, double> (docid, weight));
        // if(map_iter == weight_map.end()){
        //     //If document is not present add the document and initialize weaight
        //     weight_map.insert(pair<int,double>(docid,weight));
        // }
        // else{
        //     //else if doc is present update weight
        //     weight_map[docid] += weight;
        // }
        post_iter++;
    }
}


bool compare_weights(const pair<double,int>& pair1, const pair<double,int>& pair2){
    return pair1.first > pair2.first;
}

//Process query to get search result
// vector<pair<double, int> > query_processor(string query, fixed_name_map* termmap, inverted_index_map* invindex, docweight_map* docweightmap){

//     string remaining_query(query);
//     string term;
//     vector<unsigned int> terms_array;
//     int position = remaining_query.find(" ");
    
//     term = remaining_query.substr(0,position); 
//     terms_array.push_back(fetchtermid(term, termmap));
//     remaining_query = remaining_query.erase(0,position+1);
    
    
//     //Generate term
//     while((position = remaining_query.find(" ")) != std::string::npos){
//         term = remaining_query.substr(0,position);
//         terms_array.push_back(fetchtermid(term, termmap));

//         remaining_query = remaining_query.erase(0,position+1);
//     }

//     vector<unsigned int>::iterator iter = terms_array.begin();
    
//     int term_id;


//     map<int,double> weight_map;

    
//     //Iterate over the queries in the term and process for weight
//     while(iter!=terms_array.end()){
//         term_id = (int) *iter;
//         term_processor(term_id, weight_map, invindex, docweightmap);
//         iter++;
//     }
    
//     map<int, double>::iterator map_iter;
//     map_iter = weight_map.begin();
//     vector<pair<double,int> > ranked_list;

//     while(map_iter != weight_map.end()){
//         ranked_list.push_back(pair<double,int> (map_iter->second, map_iter->first));
//         map_iter++;
//     }

//     //sort the result on score
//     sort(ranked_list.begin(), ranked_list.end(), compare_weights);

//     return ranked_list;
// }

docweight_vector query_processor(string query, fixed_name_map* termmap, inverted_index_map* invindex, docweight_map* docweightmap){

    string remaining_query(query);
    string term;
    vector<unsigned int> terms_array;
    int position = remaining_query.find(" ");
    
    term = remaining_query.substr(0,position); 
    terms_array.push_back(fetchtermid(term, termmap));
    remaining_query = remaining_query.erase(0,position+1);
    
    
    //Generate term
    while((position = remaining_query.find(" ")) != std::string::npos){
        term = remaining_query.substr(0,position);
        terms_array.push_back(fetchtermid(term, termmap));

        remaining_query = remaining_query.erase(0,position+1);
        //TEST: cout << term << endl;
    }

    vector<unsigned int>::iterator iter = terms_array.begin();
    
    int term_id;


    docweight_vector weight_map;

    
    //Iterate overt the queries in the term
    while(iter!=terms_array.end()){
        term_id = (int) *iter;
        term_processor(term_id, weight_map, invindex, docweightmap);
        iter++;
    }
    //sort based on 
    stxxl::sort(weight_map.begin(), weight_map.end(), docidcomparator(), 512*1024*1024);

    double weight;
    unsigned int docid;

    docweight_vector ranked;

    docweight_vector::iterator docidsorter_iter = weight_map.begin();
    docid = docidsorter_iter->first;
    weight = docidsorter_iter->second;
    ++docidsorter_iter;

    while(docidsorter_iter != weight_map.end())
    {
        if(docidsorter_iter->first!=docid)
        {
            ranked.push_back( pair<unsigned int, double>(docid, weight));
            docid = docidsorter_iter->first;
            weight = docidsorter_iter->second;
        }
        else
        {
            weight += docidsorter_iter->second;
        }
        ++docidsorter_iter;
    }
    
    // map<int, double>::iterator map_iter;
    // map_iter = weight_map.begin();
    // while(map_iter != weight_map.end()){
    //     ranked_list.push_back(pair<double,int> (map_iter->second, map_iter->first));
    //     map_iter++;
    // }

    stxxl::sort(ranked.begin(), ranked.end(), ranking(), 512*1024*1024);

    return ranked;
}


//Get posting list docid - termfreq pairs at seekindex
vector< pair<int, int> > seekinvertedindex(long seekindex)
{     
    vector<pair<int, int> > v;
    FILE* datafp = fopen(SEEK_INVINDEX, "rb");
    int buffer[1];
    fseek(datafp, seekindex, SEEK_SET);

    //read length of list
    fread(buffer, sizeof(int), 1, datafp);
    int* postlist = new int[(2*buffer[0])];

    //read the list
    fread(postlist, sizeof(int), 2*buffer[0], datafp);
    for(int i =0; i<buffer[0]; i++)
    {
        //puth docid - termfreq pair
        v.push_back(pair<int,int> (postlist[2*i], postlist[2*i+1]));        
    }
    delete postlist;
    fclose(datafp);
    return v;
}

//create doclength vector
void term_processor_stxxl(unsigned int term_id, docweight_vector &docidsorter, long seekindex){
    int docfreq, termfreq;
    double weight;
    unsigned int docid;
        
    cout<<"Processing term_id: "<<term_id<<endl;
    //Create vector for posting list pf the given term
    vector< pair<int,int> > post_list = seekinvertedindex(seekindex);
    //Iterator for the posting list vector
    vector< pair<int,int> >::iterator post_iter = post_list.begin();
    
    docfreq = post_list.size();
    while(post_iter != post_list.end()){ //Iterate throuh the postin list
        //get document id
        docid = post_iter->first;
        //get term frequency of the document
        termfreq = post_iter->second;
        
        //calculate the weight for the doument
        weight = calculate_tf_score(termfreq)*calculate_df_score(docfreq);

        docidsorter.push_back(pair<unsigned int, double> (docid, weight));
        post_iter++;
    }
}

//create docweightmap
docweight_map* generatedocweightmap(inverted_index_map* invindex)
{
    stxxl::syscall_file* outputfile = new stxxl::syscall_file(PERSISTENT_DOCWEIGHT_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    docweight_vector pVector(outputfile);

    docweight_map* docweight = new docweight_map((docweight_map::node_block_type::raw_size)*5, (docweight_map::leaf_block_type::raw_size)*5);
    
    docweight_vector docidsorter;

    inverted_index_map::iterator invindex_iter = invindex->begin();
    while(invindex_iter!=invindex->end())
    {        
        term_processor_stxxl(invindex_iter->first, docidsorter, invindex_iter->second);
        invindex_iter++;
    }

    stxxl::sort(docidsorter.begin(), docidsorter.end(), docidcomparator(), 512*1024*1024);

    double weight;
    unsigned int docid;

    docweight_vector::iterator docidsorter_iter = docidsorter.begin();
    docid = docidsorter_iter->first;
    weight = docidsorter_iter->second;
    ++docidsorter_iter;

    while(docidsorter_iter != docidsorter.end())
    {
        if(docidsorter_iter->first!=docid)
        {
            (*docweight)[docid] = weight;
            docid = docidsorter_iter->first;
            weight = docidsorter_iter->second;
        }
        else
        {
            weight += docidsorter_iter->second;
        }
        ++docidsorter_iter;
    }

    docweight_map::iterator docweight_iter = docweight->begin();
    while(docweight_iter!=docweight->end())
    {
        pVector.push_back(*docweight_iter);
        docweight_iter++;
    }
    return docweight;
}

//load persistent docweight map as vector
docweight_map* loaddocweightmap()
{
    stxxl::syscall_file* outputfile = new stxxl::syscall_file(PERSISTENT_DOCWEIGHT_MAP, stxxl::file::RDWR|stxxl::file::CREAT | stxxl::file::DIRECT);
    docweight_vector pVector(outputfile);

    docweight_map* docweight = new docweight_map((docweight_map::node_block_type::raw_size)*5, (docweight_map::leaf_block_type::raw_size)*5);
    docweight->insert(pVector.begin(), pVector.end());

    return docweight;
}