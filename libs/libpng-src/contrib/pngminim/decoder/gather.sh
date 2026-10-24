cp ../../pngminus/png2pnm.c pngm2pnm.c
cp ../../../*.h .
cp ../../../*.c .
rm pnggccrd.c pngvcrd.c
rm example.c pngtest.c pngpread.c pngw*.c

cp ../../../../zlib/*.h .
cp ../../../../zlib/*.c .
rm minigzip.c example.c compress.c deflate.c gz*
