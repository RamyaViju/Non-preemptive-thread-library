###################################################################
#       This is the makefile that creates a library `mythread.a`
#       This is a re-used code from reference [6]
#       
#       This file also creates the mythread.o file that must 
#       be provided while compiling the test programs.
#
#       -----------------------------------
#       Author: Ramya Vijayakumar
#       -----------------------------------
##################################################################

mythread.a: mythread.o
	ar rcs mythread.a mythread.o 

mythread.o: mythread.c mythread.h
	gcc -c -O2 mythread.c
