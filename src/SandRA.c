/*
 ============================================================================
 Name        : SandRA.c
 Author      : mpimp-golm
 Version     :
 Copyright   : CC BY-SA 4.0
 Description : Scan and Remove Adapter
 ============================================================================




- additional information, scripts, links:
	~/adapterremoval



 */



#include <time.h>
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */

#include "optlist.h"	/* parse parameters from cmd line */




// defines

#define MAXREADLEN 500

#define MAXCOUNT 5

#define READER1  50000
#define READER2 100000
#define READER3	400000
#define READER4 800000
#define WRITER1  150000




static int data = 1;


// structs

typedef struct {
	pthread_mutex_t *mut;
	int writers;
	int readers;
	int waiting;
	pthread_cond_t *writeOK, *readOK;
} rwl;

typedef struct {
	rwl *lock;
	int id;
	long delay;
} rwargs;




typedef struct pe_data
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
} pedata;



typedef struct se_data
{
/*
 * struct to hold read data to be passed to the threads
 */
    //int thread_no;
    char readID1[MAXREADLEN];
    char read1[MAXREADLEN];
    char phred1[MAXREADLEN];
} sedata;






// prototypes

rwl *initlock(void);
void readlock(rwl *lock, int d);
void writelock(rwl *lock, int d);
void readunlock(rwl *lock);
void writeunlock(rwl *lock);
void deletelock(rwl *lock);
rwargs *init_thread_data (rwl *l, int i, long d);
void *read_processing_thread (void *args);
void *read_providing_thread (void *args);










// functions



void print_help()
{
	/*
	 * say hello, say what to provide...
	 *
	 *TODO: adopt to our needs ;)
	 */
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


void remove_newline(char * line)
{
  int len = (int)strlen(line) - 1;
  if(line[len] == '\n') { line[len] = 0; }
  return ;
}





sedata** get_random_entry(FILE *fp, unsigned int N) //, char** seqlist)
{
	/*
	 *
	 * open file and get N-many random entries
	 *
	 * working version
	 *
	 */


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



	sedata** struclist = (sedata**)malloc(sizeof(sedata*) * N);
	char currline[MAXREADLEN];


    int x;
    for (x = 0; x < N; ++x)
    {
        //@DEBUG: oh!
        unsigned long seekpos =(rand() % (file_length - 1000)); //don't go beyond end of file - ensure >5 maxlen lines
        //printf("%d\t%lu\n", x,  seekpos);
        fseek(fp, seekpos, SEEK_SET);
        while (1)
        {
            if (NULL == fgets(currline, sizeof(currline), fp)){break;}
            else{
                //printf("line1 %s\n", line1);
                if ('+'==currline[0] && strlen(currline)==2)
                {
                    fgets(currline, MAXREADLEN-1, fp); //skip
                    fgets(currline, MAXREADLEN-1, fp); //first line with ^@ID

                    if ('@'==currline[0])
                    {
                		sedata *data1=(sedata*)malloc(sizeof(sedata));

                		strcpy(data1->readID1, currline);
                		if (NULL == fgets(data1->read1, MAXREADLEN-1, fp)){break;}
                		if (NULL == fgets(data1->phred1, MAXREADLEN-1, fp)){break;} // skip this line
                		if (NULL == fgets(data1->phred1, MAXREADLEN-1, fp)){break;}

                		remove_newline(data1->readID1);
                		remove_newline(data1->read1);
                		remove_newline(data1->phred1);

                		struclist[x]=data1;
                        break;
                    }
                }
            }
        }
    }

    return struclist;
}

















pedata** get_next_reads_to_process(FILE *fpR1, FILE *fpR2, unsigned int N)
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
 * TODO breaks need to be replaced by proper error handling
 *
 */
{

	pedata** struclist = (pedata**)malloc(sizeof(pedata*) * N);


    char readID1[MAXREADLEN];
    char readID2[MAXREADLEN];

    char read1[MAXREADLEN];
    char read2[MAXREADLEN];

    char phred1[MAXREADLEN];
    char phred2[MAXREADLEN];


    int i;
	for  (i=0; i<N; i++)
	{
		pedata *data1=(pedata*)malloc(sizeof(pedata));
		if (NULL == fgets(data1->readID1, sizeof(readID1), fpR1)){break;}
		if (NULL == fgets(data1->read1, sizeof(read1), fpR1)){break;}
		if (NULL == fgets(data1->phred1, sizeof(phred1), fpR1)){break;} // skip the + line
		if (NULL == fgets(data1->phred1, sizeof(phred1), fpR1)){break;}

		remove_newline(data1->readID1);
		remove_newline(data1->read1);
		remove_newline(data1->phred1);

		if (fpR2!=NULL)
		{
			if (NULL == fgets(data1->readID2, sizeof(readID2), fpR2)){break;}
			if (NULL == fgets(data1->read2, sizeof(read2), fpR2)){break;}
			if (NULL == fgets(data1->phred2, sizeof(phred2), fpR2)){break;} // skip the + line
			if (NULL == fgets(data1->phred2, sizeof(phred2), fpR2)){break;}

			remove_newline(data1->readID2);
			remove_newline(data1->read2);
			remove_newline(data1->phred2);
		}

        data1->processed=0;
		data1->thread_no = N;

		struclist[i]=data1;
	}

	return struclist;
}










/*****************
 *
 * begin trimming stuff
 */

static const char typenames[4][10] = {
	{"Phred"},
	{"Sanger"},
	{"Solexa"},
	{"Illumina"}
};

static const int quality_constants[4][3] = {
  /* offset, min, max
   *
 S - Sanger        Phred+33,  raw reads typically (0, 40)
 X - Solexa        Solexa+64, raw reads typically (-5, 40)
 I - Illumina 1.3+ Phred+64,  raw reads typically (0, 40)
 J - Illumina 1.5+ Phred+64,  raw reads typically (3, 40)
     with 0=unused, 1=unused, 2=Read Segment Quality Control Indicator (bold)
     (Note: See discussion above).
 L - Illumina 1.8+ Phred+33,  raw reads typically (0, 41)
 */



  {0, 4, 60}, /* PHRED */
  {33, 33, 126}, /* SANGER */
  {64, 58, 112}, /* SOLEXA; this is an approx; the transform is non-linear */
  {64, 64, 110} /* ILLUMINA */
};



void trim_read(pedata *readpair, int qualtype)
{
	/*
	 * do quality-based read trimming
	 *
	 *
	 *TODO to be implemented.
	 *
	 *
	 * adopted from sickle
	 * sickle says:
     Return the adjusted quality, depending on quality type.

     Note that this uses the array in sickle.h, which *approximates*
     the SOLEXA (pre-1.3 pipeline) qualities as linear. This is
     inaccurate with low-quality bases.
	 */

	#define Q_OFFSET 0
	#define Q_MIN 1
	#define Q_MAX 2

	puts("trimming");
    printf(": %s", readpair->readID1);
    printf(": %s", readpair->read1);
    printf(": %s", readpair->phred1);

	int pos;
	int qualvalue;

	int * quallist = (int*)malloc(sizeof(int)*strlen(readpair->phred1)) ;

	char qualchar;
	for (pos=0; pos<strlen(readpair->phred1); pos++)
	{
		qualchar=readpair->phred1[pos];
		qualvalue = (int) qualchar;
		printf("%c\t%d\n", qualchar, qualvalue);

		if (qualvalue < quality_constants[qualtype][Q_MIN] || qualvalue > quality_constants[qualtype][Q_MAX]) {
			fprintf (stderr, "ERROR: Quality value (%d) does not fall within correct range for %s encoding.\n", qualvalue, typenames[qualtype]);
			fprintf (stderr, "Range for %s encoding: %d-%d\n", typenames[qualtype], quality_constants[qualtype][Q_MIN], quality_constants[qualtype][Q_MAX]);
			fprintf (stderr, "Read ID: %s\n", readpair->readID1);
			fprintf (stderr, "FastQ record: %s\n", readpair->read1);
			fprintf (stderr, "Quality string: %s\n", readpair->phred1);
			fprintf (stderr, "Quality char: '%c'\n", qualchar);
			fprintf (stderr, "Quality position: %d\n", pos+1);
			exit(1);
		  }
		qualvalue -= quality_constants[qualtype][Q_OFFSET];


		quallist[pos]=qualvalue;
	}




	return;
}


/*****************
 *
 * end trimming stuff
 */












int main(int argc, const char** argv)
{

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




    unsigned short number_of_threads=5;

    srand(time(NULL));




	pthread_t r1, r2, r3, r4, w1;
	rwargs *a1, *a2, *a3, *a4, *a5;
	rwl *lock;

	lock = initlock();
	a1 = init_thread_data (lock, 1, WRITER1);
	pthread_create (&w1, NULL, read_providing_thread, a1);



	a2 = init_thread_data (lock, 1, READER1);
	a3 = init_thread_data (lock, 2, READER2);
	a4 = init_thread_data (lock, 3, READER3);
	a5 = init_thread_data (lock, 4, READER4);

	pthread_create (&r1, NULL, read_processing_thread, a2);
	pthread_create (&r2, NULL, read_processing_thread, a3);
	pthread_create (&r3, NULL, read_processing_thread, a4);
	pthread_create (&r4, NULL, read_processing_thread, a5);

	pthread_join (w1, NULL);
	pthread_join (r1, NULL);
	pthread_join (r2, NULL);
	pthread_join (r3, NULL);
	pthread_join (r4, NULL);


	//finalize
	free(a1);
	free(a2);
	free(a3);
	free(a4);
	free(a5);






	return 0;













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






    if (0)
    {

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // get random entries
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    FILE *fnrnd = fopen(fn1, "r");

    sedata ** c=NULL;
    c = get_random_entry(fnrnd, 5);

    int a2;
    for (a2 = 0; a2 < 4; a2++)
    {
        //int thread_no;
        //char [MAXREADLEN];
    	printf(">>%i\n", a2);
        printf(": %s", c[a2]->readID1);
        printf(": %s", c[a2]->read1);
        printf(": %s", c[a2]->phred1);
    }

    fclose(fnrnd);




    return EXIT_SUCCESS;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // trim reads
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FILE *fn1p = fopen(fn1, "r");
    FILE *fn2p = fopen(fn2, "r");

    pedata ** c2=NULL;
    c2 = get_next_reads_to_process(fn1p,fn2p, 3);
    puts("::::::::::::::::::");
    c2 = get_next_reads_to_process(fn1p,fn2p, 20);




    int qualtype = 3; //assuming illumina


    int a22;
    for (a22 = 0; a22 < 4; a22++)
    {
        //int thread_no;
        //char [MAXREADLEN];
    	printf(">>%i\n", a22);
        printf("%s", c2[a22]->readID1);


        trim_read(c2[a22], qualtype);
    }



    fclose(fn1p);
    fclose(fn2p);



    return EXIT_SUCCESS;

}














/*
 * begin thread based stuff
 */



rwargs *init_thread_data (rwl *l, int i, long d)
{
	/*
	 *
	 * init object for each thread
	 *
	 */


	rwargs *args;
	args = (rwargs *)malloc (sizeof (rwargs));
	if (args == NULL) return (NULL);
	args->lock = l;
	args->id = i;
	args->delay = d;
	return (args);
}

void *read_processing_thread (void *args)
{
	/*
	 *
	 * consumer
	 *
	 *
	 */


	rwargs *a;
	int d;

	a = (rwargs *)args;

	do {
		readlock (a->lock, a->id);
		d = data;
		usleep (a->delay);
		readunlock (a->lock);
		printf ("Trimmer %d : Data = %d\n", a->id, d);
		usleep (a->delay);
	} while (d != 0);
	printf ("Trimmer %d: Finished.\n", a->id);

	return (NULL);
}

void *read_providing_thread (void *args)
{
	/*
	 *
	 * producer
	 *
	 *
	 */


	rwargs *a;
	int i;

	a = (rwargs *)args;

	for (i = 2; i < MAXCOUNT; i++) {
		writelock (a->lock, a->id);
		data = i;
		usleep (a->delay);
		writeunlock (a->lock);
		printf ("read_providing_thread %d: Wrote %d\n", a->id, i);
		usleep (a->delay);
	}
	printf ("read_providing_thread %d: Finishing...\n", a->id);
	writelock (a->lock, a->id);
	data = 0;
	writeunlock (a->lock);
	printf ("read_providing_thread %d: Finished.\n", a->id);

	return (NULL);
}





/*
 * thread locking
 */

rwl *initlock (void)
{
	/*
	 *
	 * malloc and init with 0
	 *
	 *
	 */


	rwl *lock;
	lock = (rwl *)malloc (sizeof (rwl));
	if (lock == NULL) return (NULL);

	lock->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	if (lock->mut == NULL) { free (lock); return (NULL); }

	lock->writeOK =		(pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	if (lock->writeOK == NULL) { free (lock->mut); free (lock); return (NULL); }

	lock->readOK =		(pthread_cond_t *) malloc (sizeof (pthread_cond_t));
	if (lock->writeOK == NULL) { free (lock->mut); free (lock->writeOK); free (lock); return (NULL); }

	pthread_mutex_init (lock->mut, NULL);
	pthread_cond_init (lock->writeOK, NULL);
	pthread_cond_init (lock->readOK, NULL);
	lock->readers = 0;
	lock->writers = 0;
	lock->waiting = 0;

	return (lock);
}

void readlock (rwl *lock, int d)
{
	pthread_mutex_lock (lock->mut);
	if (lock->writers || lock->waiting)
	{
		do {
			printf ("reader %d blocked.\n", d);
			pthread_cond_wait (lock->readOK, lock->mut);
			printf ("reader %d unblocked.\n", d);
		} while (lock->writers);
	}
	lock->readers++;
	pthread_mutex_unlock (lock->mut);

	return;
}

void readunlock (rwl *lock)
{
	pthread_mutex_lock (lock->mut);
	lock->readers--;
	pthread_cond_signal (lock->writeOK);
	pthread_mutex_unlock (lock->mut);
}


void writelock (rwl *lock, int d)
{
	pthread_mutex_lock (lock->mut);
	lock->waiting++;
	while (lock->readers || lock->writers) {
		printf ("read_providing_thread %d blocked.\n", d);
		pthread_cond_wait (lock->writeOK, lock->mut);
		printf ("read_providing_thread %d unblocked.\n", d);
	}
	lock->waiting--;
	lock->writers++;
	pthread_mutex_unlock (lock->mut);

	return;
}

void writeunlock (rwl *lock)
{
	pthread_mutex_lock (lock->mut);
	lock->writers--;
	pthread_cond_broadcast (lock->readOK);
	pthread_mutex_unlock (lock->mut);
}

void deletelock (rwl *lock)
{
	pthread_mutex_destroy (lock->mut);
	pthread_cond_destroy (lock->readOK);
	pthread_cond_destroy (lock->writeOK);
	free (lock);

	return;
}

/*
 * end thread based stuff
 */
