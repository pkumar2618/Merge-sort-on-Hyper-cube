# Merge-sort-on-Hyper-cube
This Merge-sort Algorithm runs on 2^n nodes which are connected in a Hypercube network. I have used Message Passing Interface (MPI)
compile:
(unix shell)$ mpicc mergesort_hypercube.c -lm
(-lm is maths library linker)

Run:
(unix shell)$ mpirun -np 8 a.out 60
(number of nodes 8; 60 is the size of array, array is generated internally using rand() funcion)
