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

#include "seqfun.h"



void remove_newline(char * line)
{
  int len = (int)strlen(line) - 1;
  if(line[len] == '\n') { line[len] = 0; }
  return ;
}




char* reverse_complement(char* seq)
{
	/*
	 *
	 * non-inplace version of reverse complement
	 *
	 */
	char * rc = (char*)malloc(sizeof(char)*strlen(seq));
	int i,j;
	j=0;
	for(i=(int)strlen(seq)-1; i>=0; --i)
	{
		switch(seq[i])
		{
			case 'A': rc[j++]='T'; break;
			case 'C': rc[j++]='G';break;
			case 'G': rc[j++]='C';break;
			case 'T': rc[j++]='A';break;
			//todo errorhandling
			default: return NULL;
		}
	}
	return rc;
}




void trim_start(char ** read, char ** phred, int n)
{
	/*
	 * remove k leading chars
	 * Clips n characters from 5'-end.
	 */
	int len = (int)strlen(*read);
	*read += MIN(MAX(0, n), len - 1); // correct offset?
	*phred += MIN(MAX(0, n), len - 1); // correct offset?
	return ;
}

void trim_end(char ** read, char ** phred, int n)
{
	/*
	 * remove k trailing chars
	 */

	int len = (int)strlen(*read);
	*(*read + MIN(MAX(0, len-n), len)) = 0;
	*(*phred + MIN(MAX(0, len-n), len)) = 0;

	return ;
}





void crop_start(char** read, char** phred, int n)
{
	/*
	 * crop head of sequences to specified length
	 * move start to pos n
	 */

	int len = (int)strlen(*read);
	*read += MIN(MAX(0, len-n), len - 1); // correct offset?
	*phred += MIN(MAX(0, len-n), len - 1); // correct offset?
	return ;

}

void crop_end(char ** read, char ** phred, int n)
{
	/*
	 * Sets read length to n.
	 * crop 3'ends of sequences to specified length
	 */
	int len = (int)strlen(*read);
	*(*read + MIN(MAX(0, n), len)) = 0;
	*(*phred + MIN(MAX(0, n), len)) = 0;
	return ;
}



int is_valid_character(char c)
{
	switch(c)
	{
		case 'A': break;
		case 'C': break;
		case 'G': break;
		case 'T': break;

//TODO case sensitive
		case 'a': break;
		case 'c': break;
		case 'g': break;
		case 't': break;
		default: return 0;
	}
	return 1;
}




int valid_characters(char * line)
{
	int pos;
	for (pos=0; pos<strlen(line); pos++)
	{
		if (!is_valid_character((line[pos])))
		{
		      return 0;
		}
	}
	return 1;
}


void strupr(char **s)
{
	//TODO not working
	int pos;

/*
	for (pos=0; pos<strlen(*s); pos++)
	{
		printf("> %c\n", **s);
		**s++;
//		printf("%c\n", toupper((*s)[pos]));
		//s[pos]=toupper(s[pos]);
	}



//	while(**s++=toupper(**s))

*/

	for (pos=0; pos<strlen(*s); pos++)
	{
		printf("%d %d\n" , strlen(*s), pos);
		printf("%c\n", (*s)[pos]);
		printf("%c\n", toupper((*s)[pos]));
		//(*s)[pos]=toupper((*s)[pos]);
	}

    return;
}





void crop_to_valid_end(char* read, char* phred)
{
	/*
	 * remove leading section of read
	 * [xxNxxxN]acgtagctgc
	 *
	 */

	int pos;
	int invalidpos=0;

	//find last invalid char
	for (pos=0; pos<strlen(read); pos++)
	{
		if (!is_valid_character(read[pos]))
		{
			invalidpos=pos;
		}
	}


	if (invalidpos)
	{
		crop_end(&read, &phred, invalidpos);
	}
}




void crop_to_valid_start(char* read, char* phred)
{
	/*
	 * remove trailing section of read
	 * acgtagctgc[NxxxNxxNxx]
	 *
	 */
	int pos;

	//find first invalid char
	for (pos=0; pos<strlen(read); pos++)
	{
		//printf("%d\t%c\n", pos, read[pos]);
		if (!is_valid_character(read[pos]))
		{
			//printf("%d %c\n", pos, read[pos]);
			crop_end(&read, &phred, pos);
			return;
		}
	}

	return;
}





void trim_to_longest_valid_section(char* read, char* phred)
{
	/*
	 * find longest stretch of [ATCG]
	 *
	 */
	int pos;

	int longeststart=0;
	int longestend=0;

	int tmplongeststart=0;
	int tmplongestend=0;


	//determine longest continuous section
	for (pos=0; pos<strlen(read); pos++)
	{
		//printf("%d\t%c\n", pos, read[pos]);
		if (is_valid_character(read[pos]))
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

	printf("N-based trimming to [%d:%d] %d\n", longeststart, longestend, longestend-longeststart);
	return;
}




