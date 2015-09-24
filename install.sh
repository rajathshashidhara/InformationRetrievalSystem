echo "Installing Python Modules"

python setup_python.py
mkdir include
mkdir prog_data
mkdir prog_data/persistent
mkdir prog_data/seek_data
mkdir temp
mkdir temp/aux_index
mkdir temp/aux_index/DOCPATH
mkdir temp/aux_index/WPATH
mkdir data
mkdir data/html
mkdir data/txt
sh shell_del.sh

echo "Installing STXXL"
cd include
git clone http://github.com/stxxl/stxxl.git stxxl
cd stxxl
mkdir release 
cd release
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON ..
make

cd ../../..
make all
