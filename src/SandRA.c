/*
 ============================================================================
 Name        : SandRA.c
 Author      : mpimp-golm
 Version     :
 Copyright   : CC BY-SA 4.0
 Description : Scan and Remove Adapter
 ============================================================================
 */



#include <time.h>
#include <string.h>

#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */

#include "optlist.h"	/* parse parameters from cmd line */


#define MAXREADLEN 500



typedef struct str_thdata
{
/*
 * struct to hold read data to be passed to the threads
 */
    int thread_no;
    char readID1[MAXREADLEN];
    char readID2[MAXREADLEN];

    char read1[MAXREADLEN];
    char read2[MAXREADLEN];

    char phred1[MAXREADLEN];
    char phred2[MAXREADLEN];

    int processed;
} thdata;





void print_help()
{
    printf("SandRA Version ###\n\n");
    printf("Usage: SandRA <options>\n");
    printf("options:\n");
    printf("  -a : option excepting argument.\n");
    printf("  -b : option without arguments.\n");
	printf("  -c : option without arguments.\n");
	printf("  -d : option excepting argument.\n");
	printf("  -e : option without arguments.\n");
	printf("  -f : option without arguments.\n");
	printf("  -?  : print out command line options.\n\n");
	return;
}








int file_exists(const char * filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp)
    {
        fclose(fp);
        return 1;
    }
    return 0;
}

char ** get_random_entries(FILE *fp, unsigned int N)
{
  long int file_length;
  fseek(fp, 0, SEEK_END);

  if (fseek(fp, 0, SEEK_END)) {
    puts("Error while seeking to end of file");
    return NULL;
  }
  char ** seqlist = (char**) malloc(sizeof(char*) * N);

  file_length = ftell(fp);
  printf("file len: %li\n", file_length);

  int i = 0;
  unsigned long seekpos;

  char sep[MAXREADLEN];
  char seq[MAXREADLEN];
  char qual[MAXREADLEN];
  char header[MAXREADLEN];

  while (i <= N) {
    //unsigned long seekpos =(rand() % (file_length - 5*MAXREADLEN)); //don't go beyond end of file - ensure >5 maxlen lines
    seekpos = (rand() % (file_length - 1000));
    fseek(fp, seekpos, SEEK_SET);

    while (1) {
      if (NULL == fgets(sep, sizeof(sep), fp)) { break; }
      else {
	printf("%s", sep);

	if ('+' == sep[0] && strlen(sep) == 2) {
	  fgets(header, MAXREADLEN-1, fp);
	  fgets(header, MAXREADLEN-1, fp);

	  if ('@' == header[0]) {
	    //got a valid entry
	    fgets(seq, MAXREADLEN-1, fp);
	    fgets(sep, MAXREADLEN-1, fp);
	    fgets(qual, MAXREADLEN-1, fp);
	    printf("^ is valid entry.\nSeq is:%s", seq);

	    seqlist[i] = malloc(sizeof(char) * (strlen(seq) + 1));
	    strncpy(seqlist[i], seq, strlen(seq));
	    // seqlist[i][strlen(seq)] = '\0';
	    //seqlist[i] = strdup(seq);
	    printf("XX:%s\n", seqlist[i]);
	    ++i;



	    break;
	  }
	}
      }


    }
  }

  return seqlist;

}




char ** get_random_entry(FILE *fp, unsigned int entrynum) //, char** seqlist)
{
    long int file_length;

    fseek(fp, 0, SEEK_END);
    if(fseek(fp, 0, SEEK_END))
    {
        puts("Error while seeking to end of file");
        return NULL;
    }
    file_length = ftell(fp);

    printf("file len: %li\n", file_length);


    srand(time(NULL));


    //char** seqlist = (char**)malloc(sizeof(char*) * entrynum);
    char** seqlist = (char**)malloc(sizeof(char*) * entrynum);

    char line1[MAXREADLEN];
    char line2[MAXREADLEN];
    char line3[MAXREADLEN];
    //char * line3=(char*)malloc(MAXREADLEN);
    char line4[MAXREADLEN];
    char line5[MAXREADLEN];

    int x;
    for (x = 0; x < entrynum; x++)
    {
        //@DEBUG: oh!
        unsigned long seekpos =(rand() % (file_length - 1000)); //don't go beyond end of file - ensure >5 maxlen lines
        //printf("%d\t%lu\n", x,  seekpos);
        fseek(fp, seekpos, SEEK_SET);
        while (1)
        {
            if (NULL == fgets(line1, sizeof(line1), fp)){break;}
            else{
                //printf("line1 %s\n", line1);
                if ('+'==line1[0] && strlen(line1)==2)
                {
                    fgets(line2, MAXREADLEN-1, fp); //skip
                    fgets(line2, MAXREADLEN-1, fp); //first line with ^@ID

                    if ('@'==line2[0])
                    {
                        //got a valid entry
                        fgets(line3, MAXREADLEN-1, fp); //nucl seq
                        fgets(line4, MAXREADLEN-1, fp); //+
                        fgets(line5, MAXREADLEN-1, fp); //qual seq

                        seqlist[x]=strdup(line3);
                        //seqlist[x]=line3;
//                        printf("l3 %s\n", line3);
//                        printf("sl %s\n", seqlist[x]);
                        /*
                        sprintf((*seqlist)[x], "%d", x); //line3;
                        printf("l3 %s\n", line3);
                        printf("sl %s\n", (*seqlist)[x]);
                        int a;
                        for (a = 0; a < x; a++)
                        {
                            printf("%i\t", a);
                            printf("%s", seqlist[a]);
                        }

*/

                        break;


                    }

                }

            }
        }

    }

    return seqlist;
}

















char** get_next_reads_to_process(FILE *fpR1, FILE *fpR2, unsigned int N)
/*
 *
 * returns struct of next N-many reads fetched from files
 * entries still contain line breaks at the end
 *
 *
 * assumptions:
 * fpR1 & fpR2 are valid fastq files with seekpos pointing to current @-entry
 * fpR2 is NULL for single-end data
 *
 *
 * breaks need to be replace by proper error handling
 *
 */
{

	thdata** struclist = (thdata**)malloc(sizeof(thdata*) * N);

    char readID1[MAXREADLEN];
    char readID2[MAXREADLEN];

    char read1[MAXREADLEN];
    char read2[MAXREADLEN];

    char phred1[MAXREADLEN];
    char phred2[MAXREADLEN];


    int i;
	for  (i=0; i<N; i++)
	{
		thdata data1;
		if (NULL == fgets(data1.readID1, sizeof(readID1), fpR1)){break;}
		if (NULL == fgets(data1.read1, sizeof(read1), fpR1)){break;}
		if (NULL == fgets(data1.phred1, sizeof(phred1), fpR1)){break;}
		if (NULL == fgets(data1.phred1, sizeof(phred1), fpR1)){break;}
		if (fpR2!=NULL)
		{
			if (NULL == fgets(data1.readID2, sizeof(readID2), fpR2)){break;}
			if (NULL == fgets(data1.read2, sizeof(read2), fpR2)){break;}
			if (NULL == fgets(data1.phred2, sizeof(phred2), fpR2)){break;}
			if (NULL == fgets(data1.phred2, sizeof(phred2), fpR2)){break;}
		}

		/*
		if (NULL == fgets(readID1, sizeof(readID1), fpR1)){break;}
		if (NULL == fgets(read1, sizeof(read1), fpR1)){break;}
		if (NULL == fgets(phred1, sizeof(phred1), fpR1)){break;}
		if (NULL == fgets(phred1, sizeof(phred1), fpR1)){break;}
		if (fpR2!=NULL)
		{
			if (NULL == fgets(readID2, sizeof(readID2), fpR2)){break;}
			if (NULL == fgets(read2, sizeof(read2), fpR2)){break;}
			if (NULL == fgets(phred2, sizeof(phred2), fpR2)){break;}
			if (NULL == fgets(phred2, sizeof(phred2), fpR2)){break;}
		}

		puts("---");
		printf("%s%s\n", readID1, readID2);
		printf("%s%s\n", read1, read2);
		printf("%s%s\n", phred1, phred2);
		*/

        data1.processed=0;
		data1.thread_no = N;

		//struclist[i]=data1;

		printf("%s%s\n", data1.readID1, data1.readID2);
		printf("%s%s\n", data1.read1, data1.read2);
		printf("%s%s\n", data1.phred1, data1.phred2);

	}

	return 0;
}










void *do_loop(void *ptr)
{
    thdata *data;
    data = (thdata *) ptr;  /* type cast to a pointer to thdata */


    int me = data->thread_no;     /* thread identifying number */

    int d = rand() % me;
    ++d;
    printf("id '%d': delay: %d\n", me, d);
    nanosleep((struct timespec[]){{d, 0}}, NULL);

    /* do the work */
    printf("Thread %d read1 %s \n", data->thread_no, data->read1);
    printf("Thread %d read2 %s \n", data->thread_no, data->read2);


    //ptr
    data->processed=1;


    /* terminate the thread */
    pthread_exit(NULL);
}








int main(int argc, const char** argv) {

    unsigned short number_of_threads=5;

    srand(time(NULL));

    puts("main");




    puts("#####################\n#####################\n#####################\n#####################\n");
/*
    //TODO warum geht das nicht mit [number_of_threads]
	//int curr_threads[10];
	pthread_t thread_id[10];
	thdata data[10];

	//int i, j,k;
	int k;

	   for(k=0; k < number_of_threads; k++)
	   {
           data[k].thread_no = k;
           //printf("%d", k);

           data[k].processed=0;
           char * sd;
           sprintf(sd, "X %d", k*3);
           //puts(sd);
           strcpy(data[k].read1, sd);
           strcpy(data[k].read2, sd);

           printf(">>> %d %s  %s\n", k, data[k].read1, data[k].read2);


	      //pthread_create( &thread_id[k], NULL, do_loop, (void*)&data[k]);
	   }

	   puts("fff");

	   / *
	   for(k=0; k < number_of_threads; k++)
	   {
	      pthread_join( thread_id[k], NULL);
	   }

	   sleep(10);
*/


	int k;
        for (k=0; k<number_of_threads;k++)
        {

        	thdata data1;
            pthread_t  p_thread;       // thread's structure

            data1.thread_no = k;
            printf("%d", k);

            data1.processed=0;
            char * sd;
            sprintf(sd, "Hello! %d", k);
            puts(sd);
            strcpy(data1.read1, sd);
            strcpy(data1.read2, sd);


            int        thr_id;         // thread ID for the newly created thread
            thr_id =
            //curr_threads[k]=
            		pthread_create(&p_thread, NULL, do_loop, (void*)&data1);


            //pthread_create (&curr_thread, NULL, (void *) &print_message_function, (void *) &curr_data);
            /* Main block now waits for both threads to terminate, before it exits
               If main block exits, both threads exit, even if the threads have not
               finished their work */
            //pthread_join(thr_id, NULL);
        }
        sleep(1);
        //int b = 1;
        //do_loop((void*)&b);

        puts("done!");







        return EXIT_SUCCESS;

/*

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // THREADING EXAMPLE
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //while(1)
    {
		int k;
		for (k=0; k<number_of_threads;k++)
		{

			thdata data1;
			pthread_t  p_thread;       // thread's structure

			data1.thread_no = k;
			printf("%d", k);

			data1.processed=0;
			char * sd;
			sprintf(sd, "Hello! %d", k);
			puts(sd);
			strcpy(data1.read1, sd);
			strcpy(data1.read2, sd);


			// thread ID for the newly created thread
			//curr_threads[k] =
			int thr_id;
			thr_id=		pthread_create(&p_thread, NULL, do_loop, (void*)&data1);


			//pthread_create (&curr_thread, NULL, (void *) &print_message_function, (void *) &curr_data);
			/* Main block now waits for both threads to terminate, before it exits
			   If main block exits, both threads exit, even if the threads have not
			   finished their work * /
		//    pthread_join(thr_id, NULL);
		}
		int b = 1;
		do_loop((void*)&b);



		/*
	    puts("next round!");

	    for (k=0; k<number_of_threads;k++)
	    {
	    	printf("%d", curr_threads[k]);

	    }
		 * /
		//break;


    }

*/







    return EXIT_SUCCESS;


























    //TODO ENABLE WHEN DONE
    if (0) {
    option_t *optList, *thisOpt;

    /* get list of command line options and their arguments */
    optList = NULL;
    optList = GetOptList(argc, argv, "a:bcd:ef?");


    if (optList==NULL)
    {
    	printf("No parameters given to run the program.\n");
    	print_help();
		return EXIT_FAILURE;
    }
    else
    {
		/* display results of parsing */
		while (optList != NULL)
		{
			thisOpt = optList;
			optList = optList->next;

			if ('?' == thisOpt->option)
			{
		    	print_help();
				FreeOptList(thisOpt);   /* free the rest of the list */
				return EXIT_SUCCESS;
			}


			//parse user input
			printf("found option %c\n", thisOpt->option);
			if (thisOpt->argument != NULL)
			{
				printf("\tfound argument %s", thisOpt->argument);
				printf(" at index %d\n", thisOpt->argIndex);
			}
			else
			{
				printf("\tno argument for this option\n");
			}


			free(thisOpt);    /* done with this item, free it */
		}
    }
    }



      //char *fn1 = argv[1];
    char * fn1="/home/thieme/eclipse-workspace/SandRA/test.fastq";
    char * fn2="/home/thieme/eclipse-workspace/SandRA/test.fastq";
/*
      char *fn2 = argv[2];
      char *fo1 = argv[3];
      char *fo2 = argv[4];

      FILE *fp1 = fopen(fn1, "r");
      FILE *fp2 = fopen(fn2, "r");
      FILE *fpo1 = fopen(fo1, "w");
      FILE *fpo2 = fopen(fo2, "w");
*/
    if (!file_exists(fn1)) {
        printf("File failed to open: %s\n", fn1);
        exit(EXIT_FAILURE);
      }
    if (!file_exists(fn2)) {
        printf("File failed to open: %s\n", fn2);
        exit(EXIT_FAILURE);
      }

    FILE *fn1p = fopen(fn1, "r");
    FILE *fn2p = fopen(fn2, "r");





    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // get random entries
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    char ** c=NULL;
    c = get_random_entry(fn1p, 5);
    //c = get_random_entries(fn1p, 5);


    int a2;
    for (a2 = 0; a2 < 4; a2++)
    {
        printf(">>%i\n", a2);
        printf("%s", c[a2]);
    }


    fclose(fn1p);
    fclose(fn2p);
*/


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // trim reads
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    char ** c=NULL;
    c = get_next_reads_to_process(fn1p,fn2p, 3);
    puts("::::::::::::::::::");
    c = get_next_reads_to_process(fn1p,fn2p, 2);


    int a2;
    for (a2 = 0; a2 < 4; a2++)
    {
        printf(">>%i\n", a2);
        printf("%s", c[a2]);
    }










    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // THREADING EXAMPLE
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
    int k;
    for (k=0; k<number_of_threads;k++)
    {

    	thdata data1;
        pthread_t  p_thread;       // thread's structure

        data1.thread_no = k;
        printf("%d", k);

        data1.processed=0;
        char * sd;
        sprintf(sd, "Hello! %d", k);
        puts(sd);
        strcpy(data1.read1, sd);
        strcpy(data1.read2, sd);


        int        thr_id;         // thread ID for the newly created thread
        thr_id =
        pthread_create(&p_thread, NULL, do_loop, (void*)&data1);


        //pthread_create (&curr_thread, NULL, (void *) &print_message_function, (void *) &curr_data);
        /* Main block now waits for both threads to terminate, before it exits
           If main block exits, both threads exit, even if the threads have not
           finished their work * /
    //    pthread_join(thr_id, NULL);
    }
    int b = 1;
    do_loop((void*)&b);

*/

    puts("done!");







    return EXIT_SUCCESS;

}








