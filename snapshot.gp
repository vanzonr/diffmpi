set term x11
set size square
#plot [0:199][0:999] 'snapshot.txt' matrix with image
plot [0:][0:] 'snapshot.txt' matrix with image
