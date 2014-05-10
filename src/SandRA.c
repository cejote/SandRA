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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


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





void trim_start(char * line1, char * line2, int rem)
{
	/*
	 * set read len to particular value
	 */
	int len = (int)strlen(line1);
    //TODO :how can this be done inplace?!
	sprintf(line1, line1+MIN(MAX(0,rem),len));
	sprintf(line2, line2+MIN(MAX(0,rem),len));
	return;
}
void trim_end(char * line1, char * line2, int rem)
{
	/*
	 * set read len to rem
	 */
	int len = (int)strlen(line1);
	line1[MIN(MAX(0,rem),len)] = 0;
	line2[MIN(MAX(0,rem),len)] = 0;
	return ;
}



void crop_start(char * line1, char * line2, int rem)
{
	/*
	 * remove rem-many leading chars
	 * move start to pos rem
	 */
	int len = (int)strlen(line1);
    //TODO :how can this be done inplace?!
	sprintf(line1, line1+MIN(MAX(0,rem),len));
	sprintf(line2, line2+MIN(MAX(0,rem),len));
	return;
}
void crop_end(char * line1, char * line2, int rem)
{
	/*
	 * remove rem-many trailing chars
	 */
	int len = (int)strlen(line1);
	line1[MIN(MAX(0,len-rem),len)] = 0;
	line2[MIN(MAX(0,len-rem),len)] = 0;
	return ;
}




int has_unwanted_characters(char * line)
{
	int pos;
	for (pos=0; pos<strlen(line); pos++)
	{
		 switch(line[pos])
		 {
		      case 'A': break;
		      case 'C': break;
		      case 'G': break;
		      case 'T': break;
		      default: return 0;
		 }
	}
	return 1;
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
            //TODO: if no +-@-combination is present we'll get stuck here


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

                		if ( !has_unwanted_characters(data1->read1))
                		{
                    		remove_newline(data1->readID1);
                    		remove_newline(data1->read1);
                    		remove_newline(data1->phred1);

                    		struclist[x]=data1;
                            break;
                		}
                		else
                		{
                			free(data1);
                		}


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

    char tmpline[MAXREADLEN];

    int i;
	for  (i=0; i<N; i++)
	{
		pedata *data1=(pedata*)malloc(sizeof(pedata));
		if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->readID1, tmpline);
		if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->read1, tmpline);
		if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;} // skip the + line
		if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->phred1, tmpline);

		//TODO insert phred conversion
		//charToPhred33


		if (fpR2!=NULL)
		{
			if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
			remove_newline(tmpline);
			strcpy(data1->readID2, tmpline);
			if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
			remove_newline(tmpline);
			strcpy(data1->read2, tmpline);
			if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;} // skip the + line
			if (NULL == fgets(tmpline, MAXREADLEN-1, fpR1)){break;}
			remove_newline(tmpline);

			//TODO insert phred conversion
			//charToPhred33


			strcpy(data1->phred2, tmpline);
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

/*
#define xSANGER		0 	//S - Sanger        Phred+33,  raw reads typically (0, 40)
#define xSOLEXA		1	//X - Solexa        Solexa+64, raw reads typically (-5, 40)
#define xILLUMINA13	2	//I - Illumina 1.3+ Phred+64,  raw reads typically (0, 40)
#define xILLUMINA15	3	//J - Illumina 1.5+ Phred+64,  raw reads typically (3, 40)    with 0=unused, 1=unused, 2=Read Segment Quality Control Indicator (bold)
#define xILLUMINA18	4	//L - Illumina 1.8+ Phred+33,  raw reads typically (0, 41)
#define xINT 		5	//Phred+33
#define xINTSOLEXA 	6	//SOLEXA int
*/


#define xPHRED33	1
#define xPHRED64	2
#define xSOLEXA 	3






int phred33char_as_prob(char c)
{
	printf("char %c is %d ", c, (int)c);
	/// Translate a Phred-encoded ASCII character into a Phred quality
	return ((int)c >= 33 ? ((int)c - 33) : 0);
}



/*
* Lookup table for converting from Solexa-scaled (log-odds) quality
* values to Phred-scaled quality values.
*/
char solToPhred[] = {
/* -10 */ 0, 1, 1, 1, 1, 1, 1, 2, 2, 3,
/* 0 */ 3, 4, 4, 5, 5, 6, 7, 8, 9, 10,
/* 10 */ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
/* 20 */ 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
/* 30 */ 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
/* 40 */ 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
/* 50 */ 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
/* 60 */ 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
/* 70 */ 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
/* 80 */ 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
/* 90 */ 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
/* 100 */ 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
/* 110 */ 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
/* 120 */ 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
/* 130 */ 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
/* 140 */ 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
/* 150 */ 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
/* 160 */ 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
/* 170 */ 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
/* 180 */ 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
/* 190 */ 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
/* 200 */ 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
/* 210 */ 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
/* 220 */ 220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
/* 230 */ 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
/* 240 */ 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
/* 250 */ 250, 251, 252, 253, 254, 255
};



int solexaToPhred(int sol)
{
	/*
	* Convert a Solexa-scaled quality value into a Phred-scale quality value.
	*
	* p = probability that base is miscalled
	* Qphred = -10 * log10 (p)
	* Qsolexa = -10 * log10 (p / (1 - p))
	*/
	if (sol>256)
	{
		fprintf (stderr, "%d is not a Solexa score.", sol);
		return 0;

	}
	if(sol < -10) return 0;
	return solToPhred[sol+10];
}






char charToPhred33(char c, int qualtype)
{
	/**
	 * Take an ASCII-encoded quality value and convert it to a Phred33
	 * ASCII char.
	 *
	 *
	 *
	 * TODO a looped wrapper should be used for error checking while reading data
		if (charToPhred33<0)
		{
			fprintf (stderr, "ERROR: Quality value (%d) does not fall within correct range for %s encoding.\n", qualvalue, typenames[qualtype]);
			fprintf (stderr, "Read ID: %s\n", readpair->readID1);
			fprintf (stderr, "FastQ record: %s\n", readpair->read1);
			fprintf (stderr, "Quality string: %s\n", readpair->phred1);
			fprintf (stderr, "Quality char: '%c'\n", qualchar);
			fprintf (stderr, "Quality position: %d\n", pos+1);
		  }
	*/


	if(c == ' ')
	{
		fprintf (stderr, "Found a space character but expected an ASCII-encoded quality values. Your quality values might be provided as integers, which is currently not supported.\n");
		return -1;
	}

	char phred33char;

	//printf(">>> %c", c);

	switch (qualtype)
	{
		case xPHRED33:
			if ((c < (int)'!') || (c > (int)'I'))
			{
				fprintf (stderr, "ASCII character %c is out of range for PHRED33." , c);
				return -1;
			}
			phred33char=c;
			// keep phred quality
			break;
		case xPHRED64:
			if ((c < (int)'@') || (c > (int)'h'))
			{
				fprintf (stderr, "ASCII character %c is out of range for PHRED64." , c);
				return -1;
			}
			// Convert to 33-based phred
			phred33char= c -(64-33);
			break;
		case xSOLEXA:
			if ((c < (int)';') || (c > (int)'h'))
			{
				fprintf (stderr, "ASCII character %c is out of range for Solexa." , c);
				return -1;
			}
			phred33char= solexaToPhred((int)c - 64) + 33;

			break;
		default:
			fprintf (stderr, "This shouln't happen.");
			return -1;
	}

	if ((phred33char<33) || (phred33char>104))
	{
		fprintf (stderr, "Got ASCII character %d which is out of range." , (int)c);
		return -1;
	}

	return phred33char;
}


void test_quality_strings()
{
	char * SANGER= "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHI";
	char * SOLEXA= ";<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefgh";
	char * ILLUMINA15= "BCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefgh";

	int pos;
	for (pos=0; pos<strlen(SANGER); pos++)
	{
		printf("%c",  charToPhred33(SANGER[pos], xPHRED33));
	}
	printf("\n");
	for (pos=0; pos<strlen(SOLEXA); pos++)
	{
		printf("%c",  charToPhred33(SOLEXA[pos], xSOLEXA));
	}
	printf("\n");
	for (pos=0; pos<strlen(ILLUMINA15); pos++)
	{
		printf("%c",  charToPhred33(ILLUMINA15[pos], xPHRED64));
	}
	printf("\n");

	return;

}





float calc_avgqual(char * phred, int qualtype)
{
	/*
	 * sum-up phred score, normalize to seq len
	 */
	int qualvalue=0;
	int pos;
	for (pos=0; pos<(int)strlen(phred); pos++)
	{
		qualvalue += (int)phred[pos];
	}
	return (qualvalue/(float)strlen(phred));
}



void trim_read(char * read, char * phred, int minqual, int qualtype)
{
	/*
	 * do quality-based read trimming based on max-coverage window
	 */
	int pos;
	int qualvalue;

	int longeststart=0;
	int longestend=0;

	int tmplongeststart=0;
	int tmplongestend=0;


	//determine longest continuous section
	char qualchar;
	for (pos=0; pos<strlen(phred); pos++)
	{
		qualchar=phred[pos];
		qualvalue = (int) qualchar;

		//printf("%d\t%c %d\t%d", pos, qualchar, qualvalue, minqual);
		if (qualvalue>=minqual)
		{
			tmplongestend=pos;

			if ((tmplongestend-tmplongeststart) >(longestend-longeststart))
			{
				longeststart=tmplongeststart;
				longestend=tmplongestend;
			}
		}
		else
		{
			tmplongeststart=pos;
			tmplongestend=pos;
		}
		//printf("\t[%d:%d]\n", longeststart, longestend);
	}


	if (longestend<strlen(phred))
	{
		//TODO woher kommt dieser offset? - speichere ich einen zyklus zu spät?
		trim_end(read, phred, longestend+1);
	}
	if (longeststart>0)
	{
		//TODO woher kommt dieser offset? - speichere ich einen zyklus zu spät?
		trim_start(read, phred, longeststart+1);
	}

	//printf("trimming to [%d:%d] %d\n", longeststart, longestend, longestend-longeststart);
	//printf("y %s\ny %s\n", read, phred);

	return;
}


/*****************
 *
 * end trimming stuff
 */












int main(int argc, const char** argv)
{



/*
	char d = charToPhred33('J', xPHRED33);
	printf("%c\t%f", d, phred33char_as_prob(d));


	//test_quality_strings();

	return 0;

*/


	if (0)
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

	}




//TODO set and check parameter to be conflict-free
    unsigned short number_of_threads=5;


    int qualtype = xPHRED33;
    int trimmingminqual = 70; //(int)'A';
    int trimstart=0;	//remove k leading chars before trimming
    int trimend=0;	//remove k trailing chars before trimming
    int cropstart=0;	//trim head of sequences to specified length
    int cropend=0;	//trim 3'ends of sequences to specified length
    int minlen=0;	//discard reads with len < k
    int avgqual=0;	//discard reads with avgqual < k






    srand(time(NULL));



    if (0)
    {

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

    pedata ** readlist=NULL;
    //c2 = get_next_reads_to_process(fn1p,fn2p, 3);
    //puts("::::::::::::::::::");
    readlist = get_next_reads_to_process(fn1p,fn2p, 20);





    int PE=0;		//do we have PE data?


    int readentrycnt;
    for (readentrycnt = 0; readentrycnt < 4; readentrycnt++)
    {
        //int thread_no;
        //char [MAXREADLEN];
    	printf(">>%i\n", readentrycnt);
        printf("%s\n", readlist[readentrycnt]->readID1);
        printf("%s\n", readlist[readentrycnt]->readID2);



        printf("1l %s\n", readlist[readentrycnt]->read1);
    	printf("1l %s\n", readlist[readentrycnt]->read2);


        if (trimstart>0)
        {
        	trim_start(readlist[readentrycnt]->read1, readlist[readentrycnt]->phred1, trimstart);
            if (PE)
            {
                trim_start(readlist[readentrycnt]->read2, readlist[readentrycnt]->phred2, trimstart);
            }
        }
        if (trimend>0)
        {
            trim_end(readlist[readentrycnt]->read1, readlist[readentrycnt]->phred1, trimend);
            if (PE)
            {
            	trim_end(readlist[readentrycnt]->read2, readlist[readentrycnt]->phred2, trimend);
            }
        }
        if (cropstart>0)
        {
            crop_start(readlist[readentrycnt]->read1, readlist[readentrycnt]->phred1, cropstart);
            if (PE)
            {
            	crop_start(readlist[readentrycnt]->read2, readlist[readentrycnt]->phred2, cropstart);
            }
        }
        if (cropend>0)
        {
            crop_end(readlist[readentrycnt]->read1, readlist[readentrycnt]->phred1, cropend);
            if (PE)
            {
            	crop_end(readlist[readentrycnt]->read2, readlist[readentrycnt]->phred2, cropend);
            }
        }




        if (trimmingminqual>0)
        {
			trim_read(readlist[readentrycnt]->read1, readlist[readentrycnt]->phred1,
					trimmingminqual, 	//cut-off value for dynamic trimming
					qualtype	//phred33/64/solexa
			);
			if (PE)
			{
				trim_read(readlist[readentrycnt]->read2, readlist[readentrycnt]->phred2,
						trimmingminqual, 	//cut-off value for dynamic trimming
						qualtype	//phred33/64/solexa
				);
			}
        }



        if (minlen>0)
        {
        	if (strlen(readlist[readentrycnt]->read1)<minlen)
        	{
        		printf("read mate1 too short\n");
        	}
        	if (PE && strlen(readlist[readentrycnt]->read2)<minlen)
        	{
        		printf("read mate2 too short\n");
        	}
        }

        avgqual=70;
        if (avgqual>0)
        {
        	//TODO ensure that avgqual-scoring matches qualtype
        	if (calc_avgqual(readlist[readentrycnt]->phred1, qualtype)<avgqual)
        	{
        		printf("read mate1 has too low quality\n");
        	}
        	if (calc_avgqual(readlist[readentrycnt]->phred2, qualtype)<avgqual)
        	{
        		printf("read mate2 has too low quality\n");
        	}
        }


        printf("1 %s\n", readlist[readentrycnt]->read1);
        printf("2 %s\n", readlist[readentrycnt]->phred1);
    	//printf("2l %s\n", readlist[readentrycnt]->read2);
    	//printf("2l %s\n", readlist[readentrycnt]->phred2);


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
