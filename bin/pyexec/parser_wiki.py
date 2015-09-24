import bs4
from bs4 import BeautifulSoup
from unidecode import unidecode
import string
import constants
import os

# Parse html document to text document and normalize the text with unidecode library
def parse_document(path):
	f = open(path)
	html_text = f.read()
	if html_text==None:
		return
	f.close()
	soup = BeautifulSoup(html_text, 'html.parser')

	if soup==None:
		return
	
	only_text = ""
	
	if soup.title!=None:
		only_text += unidecode(soup.title.string[:soup.title.string.find('-')]).lower()
		only_text += "\n"

	for p in soup.find_all('p'):
		for c in p.stripped_strings:
			c = unidecode(c).lower()
			only_text += c
			only_text += " "
	
	new_name = path[path.rfind('/')+1:]
	new_path = constants.CORPUS + '/html/' + new_name
	os.rename(path,new_path)

	otpt = open(new_path.replace('html','txt'), 'w+')
	otpt.write(only_text)
	otpt.close()

	return only_text
