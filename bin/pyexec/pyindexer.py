import subprocess
import constants
import sys
import os
import parser_wiki
import nl_proc
import inverter

#this script takes file(s) and indexes them to create termmap, docmap
#and inverted index. Then it merges the index with the already existing
#index.

subprocess.Popen('python bin/pyexec/parser_wiki.py',shell=True)
subprocess.Popen('python bin/pyexec/nl_proc.py',shell=True)

rpath = sys.argv[1]		#path of file(s) to be indexed

#if one html file
if rpath.endswith('.html'):
	only_text=parser_wiki.parse_document(rpath)
	only_text_dic = nl_proc.indexer(only_text)
	print only_text_dic
	name = rpath[rpath.rfind('/')+1:]
	print 'name: ' + name
	new_path = constants.TEMP_WPATH + name.replace('.html','.txt')
	print 'new_path: ' + new_path
	otpt = open(new_path, 'w+')
	for k in sorted(only_text_dic.keys()):
		otpt.write(str(k) + ' ' + str(only_text_dic[k])+'\n')
	subprocess.Popen('python bin/pyexec/inverter.py',shell=True)
	inverter.logarithmic_merge(constants.TEMP_WPATH)

#if one txt file
elif rpath.endswith('.txt'):
	only_text = ""
	with open(rpath) as lines:
		for line in lines:
			only_text += line
	lines.close()
	new_name = rpath[rpath.rfind('/')+1:]
	new_path = constants.CORPUS + '/txt/' + new_name
	os.rename(rpath,new_path)
	only_text_dic = nl_proc.indexer(only_text)
	new_path = constants.TEMP_WPATH + new_name
	otpt = open(new_path, 'w+')
	for k in sorted(only_text_dic.keys()):
		otpt.write(str(k) + ' ' + str(only_text_dic[k])+'\n')
	subprocess.Popen('python bin/pyexec/inverter.py',shell=True)
	inverter.logarithmic_merge(constants.TEMP_WPATH)

#if multiple html files
else:
	for root, dirs, files in os.walk(rpath):
		for name in files:
			if name.endswith('.html'):
				print 'processing: '+ name
				only_text=parser_wiki.parse_document(os.path.join(root, name))
				only_text_dic = nl_proc.indexer(only_text)
				new_path = constants.TEMP_WPATH + name.replace('.html','.txt')
				otpt = open(new_path, 'w+')
				for k in sorted(only_text_dic.keys()):
					otpt.write(str(k) + ' ' + str(only_text_dic[k])+'\n')
	subprocess.Popen('python bin/pyexec/inverter.py',shell=True)
	inverter.logarithmic_merge(constants.TEMP_WPATH)
