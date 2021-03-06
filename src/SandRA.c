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

#include <ctype.h>


#include "sfxtree.h"	/* just treeming */
#include "seqfun.h"		/*  */

// defines
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define MAXREADLEN 500
#define MAXADAPTERNUM 500

#define MAXCOUNT 15

#define MAXREADLEN 500

#define INREADCOUNT	3

#define xPHRED33	1
#define xPHRED64	2
#define xSOLEXA 	3


#define xIGNORE			0
#define xTRIMLONGEST	1
#define xTRIMSTART		2
#define xTRIMEND		3


int THREADCOUNT =10;
int READBLOCKSIZE = 11;


long unsigned int reads_processed = 0;
long unsigned int reads_trimmed3 = 0;
long unsigned int reads_trimmed5= 0;
long unsigned int reads_trimmed35 = 0;






//pthread_cond_t items = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readfilelock = PTHREAD_MUTEX_INITIALIZER;




FILE *infpR1;
FILE *infpR2;

FILE *outfpR1perfect;
FILE *outfpR2perfect;
FILE *outfpR1discarded;
FILE *outfpR2discarded;

FILE *outflog;





// structs

typedef struct pe_data
{
/*
 * struct to hold read data to be passed to the threads
 */
    int thread_no;
    char *readID1;
    char *readID2;

    char *read1;
    char *read2;

    char *phred1;
    char *phred2;

    int processed;
} pedata;






// prototypes






// global variables
    int qualtype;
    int trimmingminqual; //(int)'A';
    int trimstart;	//remove k leading chars before trimming
    int trimend;	//remove k trailing chars before trimming
    int cropstart;	//trim head of sequences to specified length
    int cropend;	//trim 3'ends of sequences to specified length
    int minlen;		//discard reads with len < k
    int avgqual;	//discard reads with avgqual < k
    int n_splitting; //get longest valid [ATCG] stretch of read

    int PE=0;		//do we have PE data?

    char** useradapters=NULL; // get list of known adapters

	int cutoffperc=90;	//poly-A detection - skip elongation when below this score
	int startlen=10;	//poly-A detection - required seed
	int startcutoff=80;	//poly-A detection - percentage within seed






// functions


void print_help()
{
	/*
	 * say hello, say what to provide...
	 *
	 */
    printf("SandRA Version ###\n\n");
    printf("Usage: SandRA <options>\n");
    printf("options:\n");
    //TODO ....
    printf("  -?  : print out command line options.\n\n");
    printf("When using, please cite:\n");
    //TODO ...
    printf("Thieme, Schudoma et al, 2014\n");

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





pedata* create_pe_data()
{
	/*
	 * init new pedata struct
	 *
	 */
	pedata *data1=(pedata*)malloc(sizeof(pedata));

	data1->readID1=(char*)malloc(MAXREADLEN*sizeof(char));
	data1->read1=(char*)malloc(MAXREADLEN*sizeof(char));
	data1->phred1=(char*)malloc(MAXREADLEN*sizeof(char));
	data1->readID2=(char*)malloc(MAXREADLEN*sizeof(char));
	data1->read2=(char*)malloc(MAXREADLEN*sizeof(char));
	data1->phred2=(char*)malloc(MAXREADLEN*sizeof(char));
	return data1;
}




pedata** get_next_reads_to_process(unsigned int N)
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

	pedata** struclist = (pedata**)calloc(N, sizeof(pedata*));



	//printf("WAIT\n");
	pthread_mutex_lock(&readfilelock);
	//pthread_cond_wait(&slots, &slot_lock);
	//printf("START\n");

	char tmpline[MAXREADLEN];
    int i;
    pedata* data1;
	for  (i=0; i<N; i++)
	{

		data1=create_pe_data();

		if (NULL == fgets(tmpline, MAXREADLEN-1, infpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->readID1, tmpline);
		if (NULL == fgets(tmpline, MAXREADLEN-1, infpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->read1, tmpline);
		if (NULL == fgets(tmpline, MAXREADLEN-1, infpR1)){break;} // skip the + line
		if (NULL == fgets(tmpline, MAXREADLEN-1, infpR1)){break;}
		remove_newline(tmpline);
		strcpy(data1->phred1, tmpline);

		if(strlen(data1->phred1) != strlen(data1->read1))
		{
			fprintf (stderr, "PHRED does not match to read: \n%s\n%s\n", data1->read1, data1->phred1);
			return NULL;
		}


		//TODO insert phred conversion
		//charToPhred33


		if (infpR2!=NULL)
		{
			if (NULL == fgets(tmpline, MAXREADLEN-1, infpR2)){break;}
			remove_newline(tmpline);
			strcpy(data1->readID2, tmpline);
			if (NULL == fgets(tmpline, MAXREADLEN-1, infpR2)){break;}
			remove_newline(tmpline);
			strcpy(data1->read2, tmpline);
			if (NULL == fgets(tmpline, MAXREADLEN-1, infpR2)){break;} // skip the + line
			if (NULL == fgets(tmpline, MAXREADLEN-1, infpR2)){break;}
			remove_newline(tmpline);
			strcpy(data1->phred2, tmpline);

			if(strlen(data1->phred2) != strlen(data1->read2))
			{
				fprintf (stderr, "PHRED does not match to read: \n%s\n%s\n", data1->read2, data1->phred2);
				return NULL;
			}

			//TODO insert phred conversion
			//charToPhred33
		}

        //data1->processed=0;
		//data1->thread_no = N;

		struclist[i]=data1;
	}
	//printf("unlocking & exit\n");
	pthread_mutex_unlock(&readfilelock);

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





void polyA_trim_read(char ** read, char ** phred, int cutoffperc, int startlen, int startcutoff)
{
	/*
	 * cutoffperc - cut when below this value
	 * startlen - start section which needs to be above startcutoff
	 * startcutoff - threshold for start section
	 *
	 * do quality-based read trimming based on max-coverage window
	 *
	 *
	 * TODO: Poly-T @ RC
	 *
	 */

	if (strlen(*read)<=10)
	{
		//TODO
		*read=NULL;
		*phred=NULL;
	}


	int acnt=0;
	int lastpos=0;
	int pos;
	for (pos=0; pos<startlen; pos++)
	{
		//printf("%c\n", (*read)[pos]);
		if ((*read)[pos]=='A')
		{
			++acnt;
			lastpos=pos; //might result in trimmed sections smaller than startlen
		}
	}


	if (100.0*((double)acnt/startlen)<startcutoff) return;

	for (pos=startlen; pos<strlen(*read); pos++)
		if ((*read)[pos]=='A')
			if (100.0*((double)++acnt/pos)>cutoffperc) lastpos=pos; //might result in trimmed sections smaller than startlen

	trim_start(read, phred, lastpos+1);
	return;
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


	if (longestend<strlen(read))
	{
		crop_end(&read, &phred, longestend+1);
	}
	if (longeststart>0)
	{
		trim_start(&read, &phred, longeststart+1);
	}

	printf("Q-based trimming to [%d:%d] %d\n", longeststart, longestend, longestend-longeststart);
	//printf("y %s\ny %s\n", read, phred);

	return;
}


/*****************
 *
 * end trimming stuff
 */






//TODO trimming tree is to be used as second parameter
void trim_adapters(char** read, char** phred)
{
	/*
	 * trim sequences
	 *
	 * uses globar var useradapters
	 *
	 */


	int i=0;
	while(NULL!=useradapters[i])
	{
		printf(">adapter_%d\n%s\n", i, *read);
		//todo: tree.
		++i;
	}

	//trimmed 3'
	++reads_trimmed3;
	//trimmed 5'
	++reads_trimmed5;
	//trimmed both ends
	++reads_trimmed35;



	return;

}









char** detect_adapters(FILE *fp)
{
	/*
	 * find potential adapter sequences using suffix tree-bases search
     * open read file and get N-many random entries
	 *
	 *
	 */
	srand(time(NULL));


	int N=100; //number of random entries to fetch

	//todo MAXADAPTERNUM - get proper length
	char** adapters = (char**)malloc(sizeof(char*) * MAXADAPTERNUM);

    int rndm=0;

	long int file_length;

	if (rndm) // go to random pos, if required
	{

		fseek(fp, 0, SEEK_END);
		if(fseek(fp, 0, SEEK_END))
		{
			puts("Error while seeking to end of file");
			return NULL;
		}
		file_length = ftell(fp);
		fprintf (stderr, "file len: %li\n", file_length);
		if (file_length<200000*10*200*3)
		{
			//200000 reads * 10 to have some choices * 200 linelen * 3 headerstring, readstring, and qualstring
			//TODO parameter
			fprintf(stderr, "Too few entries to perform an automated primer detection. Sorry, try again with more data or trim to known adapters, only (parameter -a).");
			return NULL;
		}
	}

    SFXTreeWrapper * sfx = initWrapper();
    //pedata* currread;
    //pedata** struclist = (pedata**)malloc(sizeof(pedata*) * N);

	char currline[MAXREADLEN];

    int x;
    for (x = 0; x < N; ++x)
	{
    	if (rndm) // go to random pos, if required
    	{
    		//TODO untested - might get @ from qual string?? - tested later by "+"-test
    		unsigned long seekpos =(rand() % (file_length - 1000)); //don't go beyond end of file - ensure >5 maxlen lines
    		fseek(fp, seekpos, SEEK_SET);
    		fgets(currline, MAXREADLEN-1, fp); //skip incomplete line
    	}

		//continue until @
    	currline[0]=-1;
    	while ('@'!=currline[0])
		{
			if (NULL == fgets(currline, sizeof(currline), fp)){continue;}
			printf("looping\n");
		}

		printf("header > %d %s\n", x, currline);

		pedata *data1=create_pe_data();//(pedata*)malloc(sizeof(pedata));

		strcpy(data1->readID1, currline);

		if (NULL == fgets(data1->read1, MAXREADLEN-1, fp)){continue;}

		if (NULL == fgets(data1->phred1, MAXREADLEN-1, fp)){continue;} // skip this line
		if (data1->phred1[0]!='+')
		{continue;}

		if (NULL == fgets(data1->phred1, MAXREADLEN-1, fp)){continue;}

		printf("------\n");

		remove_newline(data1->read1);
		if ( valid_characters(data1->read1))
		{
			remove_newline(data1->readID1);
			remove_newline(data1->phred1);

			strupr(&data1->read1);
			//todo: replace u,t


			printf("id %s\n", data1->readID1);
			printf("rd %s\n", data1->read1);
			printf("ph %s\n", data1->phred1);


			printf(">1 %s\n", data1->read1);


			//trimming/cropping()
			//polyA_trim_read()


			addString(&sfx, data1->read1, -(x+1));

		}


	}

    //showTree(sfx->tree, 0);

    findCommonSubstrings(sfx, 10, 5, 70);


    tearDownWrapper(sfx);

    fprintf(stderr, "done2\n");
    return adapters;
}






char** read_adapters_from_file(FILE *fp)
{
	/*
	 * read in list of user-given adapter sequences
	 *
	 * lines with ">" are ignored
	 *
	 */

	char** adapters = (char**)malloc(sizeof(char*) * MAXADAPTERNUM);
    char tmpline[MAXREADLEN];

    int cnt=0;

	while(cnt<MAXADAPTERNUM)
	{
		if (NULL == fgets(tmpline, MAXREADLEN-1, fp))
		{
			break;
		}

		if ('>' == tmpline[0])
		{
			//assume fasta header - skip line
			remove_newline(tmpline);
			//printf("SKIPPING > %s\n", tmpline);
		}
		else
		{
			remove_newline(tmpline);
			if (valid_characters(tmpline))
			{
				fprintf(stderr,"Found adapter %s\n", tmpline);
				char *data1=(char*)calloc(MAXREADLEN,sizeof(char));
				strcpy(data1, tmpline);
				adapters[cnt]=data1;
				++cnt;
			}
			else
			{
				fprintf(stderr,"Invalid adapter %s\n", tmpline);
			}
		}
	}

	return adapters;
}



int blast_adapters(char** adapters)
{
	//write data to file
	FILE *fp;
	int i;

	//create tmpfile
	fp = fopen("blastlist.fasta", "w+");
	if (fp == NULL)
	{
		fprintf(stderr, "I couldn't open blastlist.fasta for writing.\n");
		return -1;
	}

	//write qry to tmpfile
	i=0;
	while(NULL!=adapters[i])
	{
		//provide int id as adapter name
		fprintf(fp, ">%d\n%s\n", i, adapters[i]);
		++i;
	}
	fclose(fp);


	//makeblastdb -in blastlistref.fasta -dbtype nucl -out blastlistref.db
	//blastn -db blastref/blastlistref.db -num_threads 10 -outfmt "7 sseqid ssac qstart qend sstart send qseq evalue bitscore" -query blastlist.fasta -perc_identity 80



	//blast file
	FILE *blastpipe;
	//TODO evalue?
	//TODO remove from list => NULL?

	//char qseqid[200];
	int qseqid, qstart, qend, qlen, nident, sstart, send;



	blastpipe = popen("blastn -db blastref/blastlistref.db -num_threads 10 -outfmt \"6 qseqid qstart qend qlen nident sstart send\" -query blastlist.fasta -perc_identity 80 -word_size 7 -max_target_seqs 1 ", "r");
	if (blastpipe != NULL)
	{
		while (1)
		{
			char *line;
			char buf[100];
			line = fgets(buf, sizeof buf, blastpipe);
			if (line == NULL) break;
			//printf("%s", line);
			sscanf(line, "%i\t%i\t%i\t%i\t%i\t%i\t%i", &qseqid,&qstart, &qend, &qlen, &nident, &sstart, &send);
			//printf(">>> %s\t%i\t%i\t%i\t%i\t%i\t%i\n", qseqid,qstart, qend, qlen, nident, sstart, send);

			//printf("%d %d\t%d\n", qlen, nident, qlen-nident);

			//TODO coverage
			if ((double)nident/qlen>0.80)
			{
				printf("Blacklisting adapter %s\n", adapters[qseqid]);
				adapters[qseqid]=0;
			}


		}
		pclose(blastpipe);
	}
	return 0;
}



int eval_adapters(char** adapters)
{
	/*
	 * check adapters to be present within mids of input reads
	 *
	 * TODO implementation
	 *
	 * TODO using tree, exact matches, only?
	 *
	 */

	int i=0;
	while(NULL!=adapters[i])
	{
		printf("%d\n%s\n", i, adapters[i]);
		++i;
	}

	return 0;
}












void* worker(int id)
{
	//threaddata *mydata = (threaddata *)tdata;
	pedata ** readlist=NULL;

	int readentrypos;
	int running = 1;
	while(running)
	{
		readlist = get_next_reads_to_process(READBLOCKSIZE);


		readentrypos=0;
		for (readentrypos=0; readentrypos<READBLOCKSIZE;readentrypos++)
		{
			//you just can't always get what you want
			if (NULL==readlist[readentrypos])
			{
				printf("No more reads to process - exiting thread %d\n", id);
				running =0;
				break;
			}


			//printf(" t %d read %d: %s\n", id, readentrypos, readlist[readentrypos]->readID1);


	        printf("davor1 %s\n", readlist[readentrypos]->readID1);
	        printf("davor1 %s\n", readlist[readentrypos]->read1);
	        printf("davor1 %s\n", readlist[readentrypos]->phred1);

	        //printf("%s\n", readlist[readentrypos]->readID2);
	    	//printf("1l %s\n", readlist[readentrypos]->read2);




	        // TODO insert polyA_trim_read()



	        if (trimend>0) //needs to go first, otherwise trimstart will change sequence length
	        {
	            trim_end(&readlist[readentrypos]->read1, &readlist[readentrypos]->phred1, trimend);
	            if (PE)
	            {
	            	trim_end(&readlist[readentrypos]->read2, &readlist[readentrypos]->phred2, trimend);
	            }
	        }


	        if (trimstart>0)
	        {
	        	trim_start(&readlist[readentrypos]->read1, &readlist[readentrypos]->phred1, trimstart);
	            if (PE)
	            {
	                trim_start(&readlist[readentrypos]->read2, &readlist[readentrypos]->phred2, trimstart);
	            }
	        }


	        if (cropstart>0)
	        {
	            crop_start(&readlist[readentrypos]->read1, &readlist[readentrypos]->phred1, cropstart);
	            if (PE)
	            {
	            	crop_start(&readlist[readentrypos]->read2, &readlist[readentrypos]->phred2, cropstart);
	            }
	        }
	        if (cropend>0)
	        {
	            crop_end(&readlist[readentrypos]->read1, &readlist[readentrypos]->phred1, cropend);
	            if (PE)
	            {
	            	crop_end(&readlist[readentrypos]->read2, &readlist[readentrypos]->phred2, cropend);
	            }
	        }



	        //TODO correct pointer references?
	        if (n_splitting==xTRIMLONGEST)
	        {
	        	printf("xTRIMLONGEST\n");
	        	trim_to_longest_valid_section(readlist[readentrypos]->read1, readlist[readentrypos]->phred1);
				if (PE)
				{
					trim_to_longest_valid_section(readlist[readentrypos]->read2, readlist[readentrypos]->phred2);
				}
	        }
	        else if (n_splitting==xTRIMSTART)
	        {
	        	printf("xTRIMSTART\n");
	        	crop_to_valid_start(readlist[readentrypos]->read1, readlist[readentrypos]->phred1);
				if (PE)
				{
					crop_to_valid_start(readlist[readentrypos]->read2, readlist[readentrypos]->phred2);
				}
	        }
	        else if (n_splitting==xTRIMEND)
	        {
	        	printf("TRIMEND\n");
	        	crop_to_valid_end(readlist[readentrypos]->read1, readlist[readentrypos]->phred1);
				if (PE)
				{
					crop_to_valid_end(readlist[readentrypos]->read2, readlist[readentrypos]->phred2);
				}
	        }
	        else if (n_splitting==xIGNORE)
	        {
	        	//TODO print error message and continue
	        	;
	        }
	        else
	        {
	        	//TODO print error message and exit program??
	        	;
	        }



	        //TODO trimming tree is to be used as second parameter
	        trim_adapters(&readlist[readentrypos]->read1, &readlist[readentrypos]->phred1);
			if (PE)
			{
				trim_adapters(&readlist[readentrypos]->read2, &readlist[readentrypos]->phred2);
			}





	        if (trimmingminqual>0)
	        {
				trim_read(readlist[readentrypos]->read1, readlist[readentrypos]->phred1,
						trimmingminqual, 	//cut-off value for dynamic trimming
						qualtype	//phred33/64/solexa
				);
				if (PE)
				{
					trim_read(readlist[readentrypos]->read2, readlist[readentrypos]->phred2,
							trimmingminqual, 	//cut-off value for dynamic trimming
							qualtype	//phred33/64/solexa
					);
				}
	        }


	        if (minlen>0)
	        {
	        	if (strlen(readlist[readentrypos]->read1)<minlen)
	        	{
	        		printf("read mate1 too short\n");
	        	}
	        	if (PE && strlen(readlist[readentrypos]->read2)<minlen)
	        	{
	        		printf("read mate2 too short\n");
	        	}
	        }

	        if (avgqual>0)
	        {
	        	//TODO ensure that avgqual-scoring matches qualtype
	        	if (calc_avgqual(readlist[readentrypos]->phred1, qualtype)<avgqual)
	        	{
	        		printf("read mate1 has too low quality\n");
	        	}
	        	if (calc_avgqual(readlist[readentrypos]->phred2, qualtype)<avgqual)
	        	{
	        		printf("read mate2 has too low quality\n");
	        	}
	        }





	        printf("danach1 %s\n", readlist[readentrypos]->read1);
	        printf("danach2 %s\n", readlist[readentrypos]->phred1);
	    	//printf("2l %s\n", readlist[readentrypos]->read2);
	    	//printf("2l %s\n", readlist[readentrypos]->phred2);






			if (0==reads_processed%10 && reads_processed)
			{
				fprintf (stderr, "%lu reads processed.\n", reads_processed);
			}
			++reads_processed;


		}





        //TODO write output to file
		/*
		readentrypos=0;
		for (readentrypos=0; readentrypos<READBLOCKSIZE;readentrypos++)
		{
			fprintf(outfpR1perfect, "%s\n%s\n+\n%s\n", readlist[readentrypos]->readID1, readlist[readentrypos]->read1, readlist[readentrypos]->phred1);
			fprintf(outfpR2perfect, "%s\n%s\n+\n%s\n", readlist[readentrypos]->readID2, readlist[readentrypos]->read2, readlist[readentrypos]->phred2);

			fprintf(outfpR1discarded, "%s\n%s\n+\n%s\n", readlist[readentrypos]->readID1, readlist[readentrypos]->read1, readlist[readentrypos]->phred1);
			fprintf(outfpR2discarded, "%s\n%s\n+\n%s\n", readlist[readentrypos]->readID2, readlist[readentrypos]->read2, readlist[readentrypos]->phred2);
		}
		 */



		//printf("%d %d", readentrypos, READBLOCKSIZE);
	}

	return NULL;
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


int treetests()
{
    //int ** encodedStringList = malloc(sizeof(int*) * 10);
    char * string1 = malloc(10 * sizeof(char));
    char * string2 = malloc(10 * sizeof(char));
    char * string3 = malloc(10 * sizeof(char));

    SFXTreeWrapper * sfx = initWrapper();


    strncpy(string1, "TACCCAATG\0", 10);
    strncpy(string2, "ATACCGAAT\0", 10);
    strncpy(string3, "CGAATAATC\0", 10);


    addString(&sfx, string1, -1);
    printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
    showTree(sfx->tree, 0);
    addString(&sfx, string2, -2);
    printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
    showTree(sfx->tree, 0);
  #if 1
    addString(&sfx, string3, -3);
    printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
    showTree(sfx->tree, 0);
  #endif

    findCommonSubstrings(sfx, 2, 2, 10);

    tearDownWrapper(sfx);

    free(string1);
    free(string2);
    free(string3);
}


int functiontests()
{


	//poly-A removal
	if (1){
	    char * c;
	    char * p;
		c = "AAAAAAAAGAATAAAAAGCANATAAAAGCTGATGCTGCGTAGCTGCANTCTNG";
		p = "12345678901234567890123456789012345678901234567890123";

		polyA_trim_read(&c,&p,cutoffperc,startlen,startcutoff);
		printf(">>\n%s\n%s\n", c,p);
	}

	return 0;

	//RC
	if (1){
	//RC
	char* c1 = "GTGCTAGCTGATGCTGCGTAGCTGCAGGGG";
	char* q;
	q=reverse_complement(c1);
	printf("FW %s\n",c1);
	printf("RC %s\n",q);
	}



	//trim @N test case
	char* c = "GTGCTAGCTGATGCTGCGTAGCTGCATCTG";
	char* p = "123456789012345678901234567890";

    printf("%s\n", c);
    trim_to_longest_valid_section(c, p);
    printf("%s\n", c);

	c = "GTGCNTAGCTGATGCTGCGTAGCTGCANTCTNG";
	p = "123456789012345678901234567890060";

    printf("------\n%s\n", c);
    trim_to_longest_valid_section(c, p);
    printf("%s\n", c);




	return 0;
}





int main(int argc, const char** argv)
{
    srand(time(NULL));


    /*
    functiontests();

    return 0;



    char * p= malloc(40 * sizeof(char));
	sprintf(p, "chsahk2jsf3dlsg");

    printf("%s\n", p);
    strupr(&p);
    printf("%s\n", p);


    return 0;



    */




	/*
	//TEST: quality
	char d = charToPhred33('J', xPHRED33);
	printf("%c\t%d", d, phred33char_as_prob(d));

	//test_quality_strings();

	return 0;
	 */






    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // parse options
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    qualtype = xPHRED33;
    //trimmingminqual = 70; //(int)'A';

    trimstart=0;	//remove k leading chars
    trimend=0;		//remove k trailing chars
    cropstart=0;	//crop head of sequences to specified length
    cropend=0;	//crop 3'ends of sequences to specified length

    minlen=0;	//discard reads with len < k
    avgqual=0;	//discard reads with avgqual < k
    n_splitting=xTRIMLONGEST; //get [xIGNORE, xTRIMLONGEST, xTRIMSTART, xTRIMEND] valid [ATCG] stretch of read

    PE=0;		//do we have PE data?



	cutoffperc=90;
	startlen=10;
	startcutoff=80;





	//functiontests();
	//return 9;




    char * fn1="/home/thieme/eclipse-workspace/SandRA/test.fastq";
    char * fn2="/home/thieme/eclipse-workspace/SandRA/test.fastq";
    char * outfn1="/home/thieme/eclipse-workspace/SandRA/test2.out1.fastq";
    char * outfn2=NULL;
    char* adptfle=NULL;
    //char* adptfle="/home/thieme/eclipse-workspace/SandRA/adapters.fasta";


    //parse options
    int i;
    for(i=1;i<argc;i++)
    {
    	/*
    	if ((strcmp(argv[i], "--R1")) or (strcmp(argv[i], "--file1")))    		{fn1=argv[++i];}
    	else if ((strcmp(argv[i], "--R2")) or (strcmp(argv[i], "--file2")))   	{fn2=argv[++i];}
    	else if ((strcmp(argv[i], "--outR1")) or (strcmp(argv[i], "--out1")))   {outfn1=argv[++i];}
    	else if ((strcmp(argv[i], "--outR2")) or (strcmp(argv[i], "--out2")))   {outfn2=argv[++i];}


    	else if (strcmp(argv[i], "--trimstart"))	{trimstart=argv[++i];}
    	else if (strcmp(argv[i], "--trimend"))	{trimend=argv[++i];}
    	else if (strcmp(argv[i], "--cropstart"))	{cropstart=argv[++i];}
    	else if (strcmp(argv[i], "--cropend"))	{cropend=argv[++i];}

    	else if ((strcmp(argv[i], "--adapters"))	{adptfle=argv[++i];}


    	else if (strcmp(argv[i], "--minlen"))	{=argv[++i];}
    	else if (strcmp(argv[i], "--avgqual"))	{=argv[++i];}
    	else if (strcmp(argv[i], "--n_splitting"))
    	{
    		if ((strcmp(argv[++i], "ignore")) {qualtype = xIGNORE;}
    		else if ((strcmp(argv[++i], "trimlongest")) {qualtype = xTRIMLONGEST;}
    		else if ((strcmp(argv[++i], "trimstart")) {qualtype = xTRIMSTART;}
    		else if ((strcmp(argv[++i], "trimend")) {qualtype = xTRIMEND;}
    		else (printf("option not recognized");print_help(); return 1;)
    	}

    	else if (strcmp(argv[i], "--phred"))
    	{
    		if (strcmp(argv[++i], "phred33")) {qualtype = xPHRED33;}
    		else if (strcmp(argv[++i], "phred64")) {qualtype = xPHRED64;}
    		else if (strcmp(argv[++i], "solexa")) {qualtype = xSOLEXA;}
    		else (printf("option not recognized");print_help(); return 1;)
    	}


    	else if (strcmp(argv[i], "--help"))	{=argv[++i];}


    	else if (strcmp(argv[i], "--help") or strcmp(argv[i], "-h")) {print_help();}
    	else {print_help(); return 1;}
    	*/
    }

    //TODO set and check parameter to be conflict-free!



    // get list of known adapters
	//read adaters from file when file is given...
    if (NULL!=adptfle && file_exists(adptfle))
    {
        FILE *adptflep = fopen(adptfle, "r");
        useradapters=read_adapters_from_file(adptflep);
    }

    if (!file_exists(fn1)) {
        printf("File failed to open: %s\n", fn1);
        exit(EXIT_FAILURE);
      }
    if (!file_exists(fn2)) {
        printf("File failed to open: %s\n", fn2);
        exit(EXIT_FAILURE);
      }





    printf("okay!\n");
    //blast_adapters(useradapters);
    //eval_adapters(useradapters);
    //printf("done!\n");





    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // get random entries
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (1)
    {
    FILE *fnrnd = fopen(fn1, "r");

    /*
    pedata ** randomentries=NULL;
    randomentries = get_random_entry(fnrnd, 5);

    int a2;
    for (a2 = 0; a2 < 4; a2++)
    {
        //int thread_no;
        //char [MAXREADLEN];
    	printf(">>%i\n", a2);
        printf(": %s", randomentries[a2]->readID1);
        printf(": %s", randomentries[a2]->read1);
        printf(": %s", randomentries[a2]->phred1);
    }

     */

    char** detectedadapters;
    detectedadapters=detect_adapters(fnrnd);
    fclose(fnrnd);


    printf("done\n");
    return -5;



	//test for the blast procedure for auto-detected adapters
	blast_adapters(detectedadapters);


	//todo merge adapter lists
	//detectedadapters + useradapters
	int i=0;
	while(NULL!=useradapters[i])
	{
		printf("%d: %s\n", i, useradapters[i]);
		++i;
	}


	printf("done");

    }


    printf("next break point\n");
    return -5;



    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // trim reads
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	infpR1= fopen(fn1, "r");
	infpR2= fopen(fn2, "r");

	int thdcnt;

	pthread_t proc[THREADCOUNT];
	for (thdcnt=0; thdcnt< THREADCOUNT; thdcnt++)
	{
		//pthread_create(&proc[thdcnt], NULL, worker, init_thread_data(thdcnt, infpR1, infpR2));
		pthread_create(&proc[thdcnt], NULL, worker, thdcnt);
	}
	for (thdcnt=0; thdcnt< THREADCOUNT; thdcnt++)
	{
		pthread_join(proc[thdcnt],NULL);
	}


    fclose(infpR1);
    fclose(infpR2);

	printf("Done.");
	printf("Number of reads processed: %lu\n", reads_processed);


    return EXIT_SUCCESS;

}








