cp ../../pngminus/pnm2png.c pnm2pngm.c
cp ../../../*.h .
cp ../../../*.c .
rm pnggccrd.c pngvcrd.c
rm example.c pngtest.c pngr*.c pngpread.c

cp ../../../../zlib/*.h .
cp ../../../../zlib/*.c .
rm inf*.[ch]
rm minigzip.c example.c gz*
