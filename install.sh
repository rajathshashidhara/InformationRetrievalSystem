echo "Installing Python Modules"

python setup_python.py

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
