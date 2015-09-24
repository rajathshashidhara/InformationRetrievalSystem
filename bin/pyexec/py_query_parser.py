import subprocess
import constants
import sys
import nl_proc

#this script parses the query to generate term list to be searched in the index

subprocess.Popen('python bin/pyexec/nl_proc.py',shell=True)
query = str(sys.argv[1])
fl = nl_proc.parse_text(query)

f = open(constants.QUERY_PARSE_PATH, "w")
for x in fl:
	f.write(x +" ")

f.close()
