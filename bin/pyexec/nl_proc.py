import re
import nltk
from nltk.tokenize import word_tokenize
from nltk.stem.porter import PorterStemmer
from pybloom import BloomFilter
import string

#this script contains functions to parse, normalize, filter stop-words and stem the terms
#catculate term freq of the terms in the docs also
#it uses bloom filter to filter stop words and snowball stemmer to stem

ENGLISH_STOPWORDS = set(nltk.corpus.stopwords.words('english'))

bf = BloomFilter(10000, 0.005)
stopwords = nltk.corpus.stopwords.words('english')

snowballer = nltk.stem.snowball.EnglishStemmer()
	
for word in stopwords:
	bf.add(word.rstrip())
for word in list(string.punctuation):
	bf.add(word)

def stem(token):
    return snowballer.stem(token)

def tokenize_sentence(text):
    tokens = nltk.word_tokenize(text)
    return tokens

def filterwords(only_text):
	if len(only_text)==0:
		return []
	filtered_text = []
	extra_words = []
	for t in only_text:		
		if t=="":
			continue
		if len(t) < 2:
			continue

		st = stem(t)
		if st in bf:
			continue
		
		filtered_text.append(st)		

	return filtered_text

def freq_dist(only_text):
	dic = {}
	for w in only_text:
		if w in dic:
			dic[w] += 1
		else:
			dic[w] = 1

	return dic

def indexer(only_text):
	t = tokenize_sentence(only_text)
	t1 = []
	for b in t:
		for k in re.split('\W+', b):			
			for z in k.split('_'):
				t1.append(z)
	fl = filterwords(t1)

	only_text_dic = freq_dist(fl)
	return only_text_dic

def parse_text(only_text):
	t = tokenize_sentence(only_text)
	fl = filterwords(t)

	return fl
