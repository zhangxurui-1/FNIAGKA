export ROOT_DIR=/Users/zxr/workspace/FNIAGKA
export MIRACL_DIR=$ROOT_DIR/deps/miracl
export SOURCE_DIR=$MIRACL_DIR/source
export BUILD_DIR=$ROOT_DIR/build/miracl

# Copy miracl headers to include/miracl
rm -rf $ROOT_DIR/include/miracl
mkdir $ROOT_DIR/include/miracl
cp $MIRACL_DIR/include/* $ROOT_DIR/include/miracl/
rm $ROOT_DIR/include/miracl/mirdef.h
cp $ROOT_DIR/mirdef.h $ROOT_DIR/include/miracl/

# Build miracl core
rm -rf $BUILD_DIR/core
mkdir $BUILD_DIR/core
cd $BUILD_DIR/core

MIRACL_core_src=""
while IFS= read -r file; do
    [[ -z "$file" ]] && continue
    [[ "$file" =~ ^# ]] && continue

    [[ "$file" = "mrmuldv.c" ]] && file="mrmuldv.ccc"

    MIRACL_core_src+="$SOURCE_DIR/$file "
done < "$ROOT_DIR/miracl.lst"

echo "[INFO] MIRACL core source files: $MIRACL_core_src"

gcc -x c -O2 -DMR_NO_CPP -I$ROOT_DIR/include/miracl -fPIC -c $MIRACL_core_src

ar rcs libMIRACL-core.a *.o
mv *.a ../
