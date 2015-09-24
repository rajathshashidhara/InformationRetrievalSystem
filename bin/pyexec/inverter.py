import os
import constants

#	this script inverts term, term freq mapping of every document and
#	create posting list for it. Then it merges every document to generate
#	final posting list which then is merged with existing posting list.
#	Also creates term - termid and doc - docid map 

number = 0

def merge(filename_1,filename_2):
	global number
	print "processing: " + filename_1 + " and " + filename_2
	#Open input files
	file_1 = open(filename_1,'r')
	file_2 = open(filename_2,'r')
	
	#Generate output file
	filename_out = constants.TEMP_WPATH+"/"+"out_"+str(number)+'.txt'
	outfile = open(filename_out,'w+')
	number = number+1
	
	#read first line
	line_1 = file_1.readline()
	line_2 = file_2.readline()

	while True:

		#break loop if we reach end of both files
		if(len(line_1)==0 and len(line_2)==0): break 
		
		#append the remaining
		elif(len(line_1)==0):
			line_str = line_2[:-1]
			line_2 = file_2.readline()

		#append the remaining
		elif(len(line_2)==0):
			line_str = line_1[:-1]
			line_1 = file_1.readline()

		#compare and merge
		else:
			comp_1 = line_1[:-1].split(" ")
			comp_2 = line_2[:-1].split(" ")

			#write line_1 and read new line
			if(comp_1[0] < comp_2[0]):
				line_str = line_1[:-1]
				line_1 = file_1.readline()

			#write line_2 and read new line
			elif(comp_1[0] > comp_2[0]):
				line_str = line_2[:-1]
				line_2 = file_2.readline()

			#merge line_1 and line_2 then write
			else:				
				#calculate freq
				freq = int(comp_1[1]) + int(comp_2[1])
				
				postlist_1 = comp_1[2:]
				postlist_2 = comp_2[2:]

				#merge two lists
				n1 = len(postlist_1)
				n2 = len(postlist_2)
				i = 0
				j = 0
				out_postlist = ""

				while((i<n1) and (j<n2)):
					r1 = postlist_1[i].split(",")[0]
					r2 = postlist_2[j].split(",")[0]
					if(r1 <= r2):
						out_postlist = out_postlist + postlist_1[i].strip() + " "
						i=i+1
					else:
						out_postlist = out_postlist + postlist_2[j].strip() + " "
						j=j+1

				while(i<n1):
					out_postlist = out_postlist + postlist_1[i].strip() + " "
					i=i+1

				while(j<n2):
					out_postlist = out_postlist + postlist_2[j].strip() + " "
					j=j+1

				#output string
				line_str = comp_1[0] + " " + str(freq) + " " + out_postlist.strip()
				
				#read new line for next iteration
				line_1 = file_1.readline()
				line_2 = file_2.readline()
		
		#write output string to outfile
		outfile.write(line_str + "\n")

	#cleaning
	file_1.close()
	file_2.close()
	outfile.close()
	os.remove(filename_1)
	os.remove(filename_2)


def logarithmic_merge(rpath):
	docmap = open(constants.TERM_DOCMAP,"r")
	lines = docmap.readlines()
	if len(lines) !=0:
		docid = int(lines[-1].split(' ')[0])
	else:
		docid = 0
	docmap.close()

	print docid

	docmap_file = open(constants.TERM_DOCMAP,"a")

	#invert to get post list for every file
	for root, dirs, files in os.walk(rpath):
		for name in files:
			if name.endswith('.txt'):
				print "Processing: " + os.path.join(root, name)
				docid = docid+1
				docmap_file.write(str(docid) + " " + constants.CORPUS+'html/'+ name.replace('.txt','.html') + '\n')
				outfile = open(constants.TEMP_WPATH + str(docid)+".txt","w")
				with open(os.path.join(root, name)) as lines:
					for line in lines:						
						try:
							term,freq = line.split(' ')
							outfile.write(term + " 1 " + str(docid) + "," + freq)
						except ValueError:
							l = line.split()
							new_term = l[0]
							for i in l[1:-1]:
								new_term = new_term + "_" + i
							outfile.write(new_term + " 1 " + str(docid) + "," + l[-1] + '\n')
				lines.close()
				outfile.close()
				os.remove(os.path.join(root, name))
	docmap_file.close()

	#merge all files to get plist.txt
	flag = False
	docid = 0
	while True:
		for root, dirs, files in os.walk(constants.TEMP_WPATH):
			if len(files)>1:
				files1 = []
				for f in files:
					if f.endswith('.txt'):
						files1.append(f)
				for i in range(0,len(files1)/2):
					merge(os.path.join(root, files1[i]),os.path.join(root, files1[len(files1)/2+i]))
			else:
				flag = True				
				os.rename(os.path.join(root, files[0]), os.path.join(root, "abc.txt"))
				break				
		if flag == True: break

	merge(constants.TEMP_POSTLIST, constants.TEMP_WPATH+"abc.txt")
	os.rename(constants.TEMP_WPATH+"out_"+str(number-1)+".txt", constants.TEMP_POSTLIST)
	outfile = open(constants.TERM_TERMMAP,"w")
	docid = 0

	#create term - termid map
	with open(constants.TEMP_POSTLIST) as lines:
		for line in lines:
			terms = line.split()
			docid = docid+1
			outfile.write(terms[0]+" "+str(docid)+"\n")
	outfile.close()
