# Non-preemptive-thread-library
# mythread.a - non pre-emptive user-level threads library mythread.a

# Files used
Makefile
mythread.c
mythread.h

# Extra credit files
mythreadextra.h

# Invocation
make

[Example]
cc tree.c mythread.o
./a.out 2 2 1 1

# Compiling without make
gcc -o mythread.o -c mythread.c
ar rcs mythread.a mythread.o

# Author
Ramya Vijayakumar
Unity Id: rvijaya4
Student Id: 200263962
