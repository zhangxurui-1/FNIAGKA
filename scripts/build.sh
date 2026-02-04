export ROOT_DIR=/Users/zxr/workspace/FNIAGKA

cd $ROOT_DIR
rm -rf build/NIAGKA
mkdir build/NIAGKA
cd build/NIAGKA

cmake ../..
cmake --build . -j