import subprocess
import constants
import sys
import os
import parser_wiki
import nl_proc
import inverter

subprocess.Popen('python parser_wiki.py',shell=True)
subprocess.Popen('python nl_proc.py',shell=True)

rpath = sys.argv[1]

for root, dirs, files in os.walk(rpath):
	for name in files:
		if name.endswith('.html'):
			print 'processing: '+ name
			only_text=parser_wiki.parse_document(os.path.join(root, name))
			only_text_dic = nl_proc.indexer(only_text)
			new_path = constants.TEMP_WPATH + '/' + name.replace('.html','.txt')
			otpt = open(new_path, 'w+')
			for k in sorted(only_text_dic.keys()):
				otpt.write(str(k) + ' ' + str(only_text_dic[k])+'\n')

subprocess.Popen('python inverter.py',shell=True)
inverter_new.logarithmic_merge(constants.TEMP_WPATH)
