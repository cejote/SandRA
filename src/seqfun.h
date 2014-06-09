/*
 ============================================================================
 Name        : SandRA.c
 Author      : mpimp-golm
 Version     :
 Copyright   : CC BY-SA 4.0
 Description : Scan and Remove Adapter
 ============================================================================


 */

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))



void remove_newline(char * line);
char* reverse_complement(char* seq);

void trim_start(char ** read, char ** phred, int n);
void trim_end(char ** read, char ** phred, int n);

void crop_start(char** read, char** phred, int n);
void crop_end(char ** read, char ** phred, int n);

int is_valid_character(char c);
int valid_characters(char * line);


void strupr(char** s);


//handling unknown characters
void crop_to_valid_end(char* read, char* phred);
void crop_to_valid_start(char* read, char* phred);
void trim_to_longest_valid_section(char* read, char* phred);




