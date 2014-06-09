#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX(a,b) (((a)>(b))?(a):(b))
#define BE_QUIET 0

typedef struct SFXTree
{
  int incLblStart, incLblEnd;
  int stringID, sfxID;
  int incFirstChar;

  struct SFXTree ** children;
  int nChildren, lastChild;
  int nVisitingStrings;
  int lastVisitor;
  int strDepth;

} SFXTree;

typedef struct SFXTreeWrapper
{
  struct SFXTree * tree;

  int ** encodedStrings;
  int nStrings, maxStrings;
} SFXTreeWrapper;

typedef struct PrefixMatch
{
  int pfxIndex;
  int lblIndex;

  int result;
  struct SFXTree * next;
  struct SFXTree * last;
} PrefixMatch; 

typedef struct CommonSubstring
{
  char * string;
  int nOccurrences;  
} CommonSubstring;

typedef struct SFXStack {
  SFXTree ** nodes;
  int maxSize;
  int top;
} SFXStack;

void showNode(SFXTree * node, int level);
void initStack(SFXStack *stack, int maxSize);
void push(SFXStack * stack, SFXTree * node);
SFXTree * pop(SFXStack *stack);
int stackIsEmpty(SFXStack *stack);

void insertStringNaive(SFXTree ** st, int stringID, int slen, int ** encList, int verbose);
void showEncodedString(int * encoded, int slen);
void showNode(SFXTree * node, int level);
void visitNode(SFXTree ** node, int stringID);

SFXTree * createNode(int start, int end, int sid, int sfxid, int fc);
SFXTree * initTree();
SFXTreeWrapper * initWrapper();

int * encodeString(char * string, int stringID);
void addString(SFXTreeWrapper ** sfx, char * string, int stringID);
void tearDown(SFXTree * node);
void tearDownWrapper(SFXTreeWrapper * sfx);
void showTree(SFXTree * node, int level);

void addChild(SFXTree ** st, SFXTree * child);
SFXTree * findEdgeByFirstChar(SFXTree * node, int fc);
SFXTree * setChildByFirstChar(SFXTree * node, SFXTree * child, int fc);
PrefixMatch * matchPrefixToEdgeLabel(int ** pfx, int pfxLen, int sfxIdx, SFXTree * node, int ** encList, int stringID);

void setStringDepth(SFXTree * node, SFXTree * parent, int start, int end, int slen);
void insertStringNaive(SFXTree ** st, int stringID, int slen, int ** encList, int verbose);

void findCommonSubstrings(SFXTreeWrapper * sfx, int minlen, int k, int estReadLen);

