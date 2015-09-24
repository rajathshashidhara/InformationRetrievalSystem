echo "Deleting Corpus"
cd data/
rm -r html
mkdir html
rm -r txt
mkdir txt
cd ..

echo "Resetting Index Files"
cd temp/
> docmap.txt
> plist.txt
> termmap.txt

echo "Cleaning up program data"
cd ..
cd prog_data
cd persistent
rm -f *
cd ../seek_data
rm -f *
cd ../..
