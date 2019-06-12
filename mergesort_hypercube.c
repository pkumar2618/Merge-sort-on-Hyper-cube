#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void merge(int *a, int l, int m, int r);
void sequential_merge(int *a, int l, int r);

int main(int argc, char *argv[]) 
{
	int i,n, rank, size ;
	MPI_Status status;
	MPI_Init(&argc,&argv); //the first arg be no of element, and then enter your array
	MPI_Comm_size(MPI_COMM_WORLD, &size);//no of processors are 8, therefore size need to be always 8 (-np 8);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	n = atoi(argv[1]);//argv[0] is the name of the program itself. atoi() convert to integers
	int hypC_d=0;//find the dimension of the hypercube
	int temp_np=size;
	int tag_chunksize=10;
	while (temp_np/2)
	{
		hypC_d += 1;
		temp_np=temp_np/2;
	}
 	if (rank<size/2)
	{
		 /* "master" process collects and distributes input */
		int chunksize;
		int *a;// *b;
		int my_d;
		int my_n;
		if (rank==0)
		{
			a=malloc((n)*sizeof(int));

			/* form the unsorted array */	
			for (i=0; i<n ; i++)
			{
				//a[i]=atoi(argv[i+2]);
				a[i]=rand()%100000;
			}
			/*print the unsorted array*/
			printf("the  entered array size: %d, hypC_d: %d, master_rank: %d\n",n, hypC_d, rank);
			printf("the unsorted array is:\n");
			for (i=0; i<n; i++)
			{
				printf("%d\t", a[i]);
			}
			printf("\n");
			my_d=0;
			chunksize=n;
			my_n=n;
		}
		else //except rank=0; all other rank has to receive data before they send data to higher dimension
		{
			my_d = 1;
			int temp_np=rank;
			while (temp_np/2)
			{
				my_d += 1;
				temp_np=temp_np/2;
			}
			int source=0;
			source = rank-pow(2,my_d-1);	
			/* receive input from the privious dimension */
			tag_chunksize= 10+my_d;
			MPI_Recv(&chunksize,1,MPI_INT,source,tag_chunksize,MPI_COMM_WORLD,&status);
						
			a=malloc((chunksize)*sizeof(int));
			MPI_Recv(a,chunksize,MPI_INT,source,my_d,MPI_COMM_WORLD,&status);
			//printf("rank %d received array of size %d, from %d in dimension : %d\n",rank, chunksize, source, my_d);
			my_n=chunksize;
		}
		///now all lower nodes will send data in higher dimension
			
		chunksize=chunksize/2;
		int offset = my_n-chunksize; 
		int dest, dimension;
		for (dimension=my_d;dimension<hypC_d;dimension++)
		{
			dest=rank+pow(2,dimension);
			tag_chunksize =10 + dimension;
			MPI_Send(&chunksize,1,MPI_INT,dest,tag_chunksize+1,MPI_COMM_WORLD);
			MPI_Send(a+offset,chunksize,MPI_INT,dest,dimension+1,MPI_COMM_WORLD);
			printf("rank %d sent array of size %d to node %d, in dimension : %d\n", rank, chunksize, dest, dimension+1);		
			chunksize=chunksize/2;
			offset -= chunksize;
		}		
		int mysize=offset+chunksize;	
		chunksize=chunksize*2;	

		/* the current process will work on remaining data, left after sending*/ 
		int *my_a;
		my_a=malloc((mysize)*sizeof(int));
		for (i=0; i<mysize; i++)
		{
			my_a[i]=a[i];
		}
		
		/* the current process will do the merge sort*/
		sequential_merge(my_a,0,mysize-1);
		int source=dest;
		//tag_chunksize=10;

		// will receive sorted element, and will join with current sorted array:collapse
		for (dimension=hypC_d;dimension>my_d;dimension--)
		{
			source = rank + pow(2,dimension-1);
			int *slave_sr;
			tag_chunksize=10+dimension;
			MPI_Recv(&chunksize,1,MPI_INT,source,tag_chunksize,MPI_COMM_WORLD,&status);

			slave_sr=malloc((chunksize)*sizeof(int));
			MPI_Recv(slave_sr,chunksize,MPI_INT,source,dimension,MPI_COMM_WORLD,&status);
			printf("the  rank (%d) received sorted array size: %d from source : %d\n", rank, chunksize, source);
			int *collapse;
			collapse=malloc((mysize+chunksize)*sizeof(int));
			for(i=0;i<mysize;i++)
			{
				collapse[i]=my_a[i];
				if (i<chunksize) collapse[i+mysize]=slave_sr[i];
			}
			//call merge function
			merge(collapse, 0, mysize-1, mysize+chunksize-1);
			/////////////////////
			mysize=mysize+chunksize;
			my_a=malloc((mysize)*sizeof(int));
			for(i=0;i<mysize;i++)
			{
				my_a[i]=collapse[i];
			}
		}
		if (rank >=1)//if rank is one or more the node will have to send the final sorted data to its parent node in the same dimension
		{
			dest=rank-pow(2,my_d-1);
			tag_chunksize=10+my_d;
			MPI_Send(&mysize,1,MPI_INT,dest,tag_chunksize,MPI_COMM_WORLD);
			MPI_Send(my_a,mysize,MPI_INT,dest,my_d,MPI_COMM_WORLD);
			printf("rank %d sent sorted array of size %d, to %d in dimension : %d\n",rank, mysize, dest, my_d);		
		}
		if (rank==0)//print the final sorted output
		{
			printf("the sorted array is :\n");
			for (i=0; i<mysize; i++)
			{
				printf("%d\t",my_a[i]);
			}
			printf("\n");
		}
	}


	/* upper half node, receive unsorted data and send the sorted data communication happens only once*/
	/* receive the unsorted array*/ 
	else 
	{
		int my_d = 1,temp_np=rank;
		int chunksize ;
		while (temp_np/2)
		{
			my_d += 1;
			temp_np=temp_np/2;
			//chunksize= chunksize/2;
		}
		int source=0;
		source = rank-pow(2,my_d-1);	

		/* receive input from the lower nodes*/
		tag_chunksize=10+my_d;
		MPI_Recv(&chunksize,1,MPI_INT,source,tag_chunksize,MPI_COMM_WORLD,&status);

		int *my_a;
		my_a=malloc((chunksize)*sizeof(int));

		MPI_Recv(my_a,chunksize,MPI_INT,source,my_d,MPI_COMM_WORLD,&status);
	
		/* slave does the merges sort*/
		sequential_merge(my_a,0,chunksize-1);
		int dest=rank-pow(2,my_d-1);
		MPI_Send(&chunksize,1,MPI_INT,dest,tag_chunksize,MPI_COMM_WORLD);
		MPI_Send(my_a,chunksize,MPI_INT,dest,my_d,MPI_COMM_WORLD);
		printf("rank %d sent sorted array of size %d, to %d in dimension : %d\n",rank, chunksize, dest, my_d);
	}

MPI_Finalize();
return 0;
}


void merge(int *a, int l, int m, int r)
{

	/* Two temp arrays */
    int a1 = m - l + 1;
    int a2 = r - m;
 	int L[a1], R[a2];
 	
 	int i,j,x;
 
    /* Copy data from arrays to be merged to temp arrays*/
    for(i = 0; i < a1; i++)
        L[i] = a[l + i];
    for(j = 0; j < a2; j++)
        R[j] = a[m + 1+ j];
 
 
    i = j = 0;
    x = l;
    while (j < a1 && i < a2)
    {
        if (L[j] <= R[i])
        {
            a[x] = L[j];
            j++;
        }
        else
        {
            a[x] = R[i];
            i++;
        }
        x++;
    }
 
    /* If remaianing copy elements of left array*/
    while (j < a1)
    {
        a[x] = L[j];
        j++;
        x++;
    }
 
    /* If remaianing copy elements of right array*/
    while (i < a2)
    {
        a[x] = R[i];
        i++;
        x++;
    }
}


void sequential_merge(int *a, int l, int r)
{
	if(l-r)
	{
		int m= (l+r)/2;
		sequential_merge(a,l,m);
		sequential_merge(a,m+1,r);
		merge(a,l,m,r);
	}
}
