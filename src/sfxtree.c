#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sfxtree.h"




void tearDownStack(SFXStack * stack)
{
  int i;
  for (i = 0; i < stack->maxSize; ++i) {
    if (NULL != (stack->nodes + i)) {
      free(*(stack->nodes + i));
    }
  }
  free(stack);
}

void initStack(SFXStack *stack, int maxSize)
{
  stack->nodes = malloc(sizeof(SFXTree) * maxSize);

  if (NULL == stack->nodes) {
    fprintf(stderr, "Insufficient memory to initialize stack.\n");
    exit(1);
  }

  stack->maxSize = maxSize;
  stack->top = -1;  
}
void push(SFXStack * stack, SFXTree * node)
{
  printf("STACK: top=%i, maxSize=%i\n", stack->top, stack->maxSize);
  showNode(node, 0);
  if (stack->top >= stack->maxSize - 1) {
    int newMax = 2 * stack->maxSize;
    printf("Resizing stack %i => %i items.\n", stack->maxSize, newMax); 
    SFXTree ** tmp = realloc(stack->nodes, sizeof(SFXTree) * newMax);
    stack->nodes = tmp;
    stack->maxSize = newMax;
  }
  stack->nodes[++stack->top] = node;
}

SFXTree * pop(SFXStack *stack)
{
  return stack->nodes[stack->top--];
}

int stackIsEmpty(SFXStack *stack)
{
  return stack->top < 0;
}











void insertStringNaive(SFXTree ** st, int stringID, int slen, int ** encList, int verbose);

void showEncodedString(int * encoded, int slen) 
{
  int i;
  for (i = 0; i < slen; ++i)
    printf("%i ", encoded[i]);
  printf("(%i)\n", encoded[slen]);
  return ;
}

void showNode(SFXTree * node, int level)
{
  int i;
  for (i = 0; i < level; ++i) { printf(">"); }
  if (level > 0) { printf(" "); }

  printf("NODE: (%i, %i, %i, %i, %i), depth=%i, %i visitors(last=%i), %i/%i children\n",
         node->stringID, node->sfxID, node->incLblStart, node->incLblEnd, node->incFirstChar,
         node->strDepth,
         node->nVisitingStrings, node->lastVisitor, node->lastChild, node->nChildren);
  return ;
}

void visitNode(SFXTree ** node, int stringID)
{
  printf("Visiting (lv=%i): ", (*node)->lastVisitor); showNode(*node, 0);
  if ((*node)->lastVisitor != stringID) {
    ++(*node)->nVisitingStrings;
    (*node)->lastVisitor = stringID;
  }
  printf("After visit (lv=%i): ", (*node)->lastVisitor); showNode(*node, 0); 
  return ;
}






SFXTree * createNode(int start, int end, int sid, int sfxid, int fc)
{
  printf("Creating Node with start=%i, end=%i, stringID=%i, suffixID=%i, fc=%i/%i\n",
         start, end, sid, sfxid, fc, fc);
 
  SFXTree * st = malloc(sizeof(SFXTree));
  st->incLblStart = start;
  st->incLblEnd = end;
  st->stringID = sid; // also serves as separator!
  st->sfxID = sfxid;
  st->incFirstChar = fc;
  st->children = NULL;
  st->nChildren = 0; // holds the number of free child bins!!
  st->lastChild = 0;
  st->nVisitingStrings = 0;
  st->lastVisitor = 0;
  st->strDepth = 0;

  return st;
}

SFXTree * initTree() 
{
  return createNode(-1, -1, 0, -1, 64);
}

SFXTreeWrapper * initWrapper() 
{
  SFXTreeWrapper * sfx = malloc(sizeof(SFXTreeWrapper));
  sfx->tree = initTree();
  sfx->encodedStrings = NULL;
  sfx->nStrings = 0;
  sfx->maxStrings = 0;
 
  return sfx;
}

int * encodeString(char * string, int stringID)
{
  int slen = strlen(string);
  printf("ES: strlen=%i, slen=%i, size-of-encoded=%i\n", strlen(string), slen, slen+1);
  printf("sizeof-int=%i sizeof-char=%i\n", sizeof(int), sizeof(char));
  int i;
  int * encoded = malloc(sizeof(int) * (slen + 1));
  char * cp = string;
  for (i = 0; i < slen; i++) {
    printf("%i ", i);
    encoded[i] = (int)*(cp + i);    
  }
  printf("\n");
  encoded[slen] = stringID;
  return encoded;
}

void addString(SFXTreeWrapper ** sfx, char * string, int stringID)
{
  int slen = strlen(string);
  printf("Now inserting string %i \"%s(%i)\" (len=%i)...\nEncoded:\n", 
         stringID, string, stringID, slen + 1); 
  int * encoded = encodeString(string, stringID);
  showEncodedString(encoded, slen);

  if (NULL == (*sfx)->encodedStrings) {
    (*sfx)->maxStrings = 1;
    (*sfx)->nStrings = 0;
    (*sfx)->encodedStrings = malloc(sizeof(int*) * 2); 
    //(sfx->encodedStrings)[0] = NULL;
    //(sfx->encodedStrings)[1] = NULL;
  } else if ((*sfx)->nStrings == (*sfx)->maxStrings) {
    (*sfx)->maxStrings *= 2;
    int ** tmp = realloc((*sfx)->encodedStrings, ((*sfx)->maxStrings + 1) * sizeof(int*)); 
    (*sfx)->encodedStrings = tmp;    
  }

  printf("addS: %i %i %i\n", -stringID, (*sfx)->nStrings, (*sfx)->maxStrings);
  (*sfx)->encodedStrings[-stringID] = encoded;
  ++((*sfx)->nStrings);
  insertStringNaive(&((*sfx)->tree), stringID, strlen(string), (*sfx)->encodedStrings, !BE_QUIET);
  return ;
}

void tearDown(SFXTree * node) 
{
  int i;
  if (NULL != node->children) {
    for (i = 0; i < node->lastChild && node->nChildren > 0; ++i) {
      printf("CP3 %i/%i\n", i, node->lastChild);
      if (NULL != *(node->children + i)) {
        printf("TD: ");  showNode(*(node->children + i), 0);
        tearDown(*(node->children + i)); 
      }
    }
    printf("CP2\n");
    free(node->children); 
  }
  printf("CP4\n");
  free(node);
  return ;
}

void tearDownWrapper(SFXTreeWrapper * sfx)
{
  int i;
  for (i = 1; i < sfx->maxStrings; ++i) {
    free(*(sfx->encodedStrings + i));
  }
  free(sfx->encodedStrings);
  tearDown(sfx->tree);
  free(sfx);

  return ;
}

void showTree(SFXTree * node, int level)
{
  int i;
  showNode(node, level);
  for (i = 0; i < node->lastChild; ++i) {
    if (NULL != node->children[i]) {
      printf("%i ", i);
      showTree(node->children[i], level + 1); 
    }
  }
  return ;
}

void addChild(SFXTree ** st, SFXTree * child)
{
  int newsize = 2 * MAX(1, (*st)->nChildren);
  if (NULL == (*st)->children) {
    // case1: node was childless before
    (*st)->children = malloc(2 * sizeof(SFXTree));   
    (*st)->children[0] = NULL;
    (*st)->children[1] = NULL;
    (*st)->nChildren = 2;
  } else if (1 == (*st)->nChildren - (*st)->lastChild) {
    // case2: node is quite fertile and needs space for more children
    printf("XX: %i %i %i %i\n", (*st)->nChildren, (*st)->lastChild, newsize, sizeof(SFXTree)); 

    SFXTree ** tmp = realloc((*st)->children, newsize * sizeof(SFXTree));
    (*st)->children = tmp;
    (*st)->nChildren = newsize;
  } 

  (*st)->children[(*st)->lastChild++] = child;  
  return ;
}

SFXTree * findEdgeByFirstChar(SFXTree * node, int fc) 
{
  int i;
  for (i = 0; i < node->lastChild; ++i) {
    if (fc == (node->children[i])->incFirstChar)
      return node->children[i];
  }
  return NULL;
}

SFXTree * setChildByFirstChar(SFXTree * node, SFXTree * child, int fc) 
{
  int i;
  SFXTree * oldChild = NULL;
  for (i = 0; i < node->lastChild; ++i) {
    if (fc == (node->children[i])->incFirstChar) {
      oldChild = node->children[i];
      node->children[i] = child;
    }
  }
  return oldChild;
}

PrefixMatch * matchPrefixToEdgeLabel(int ** pfx, int pfxLen, int sfxIdx, SFXTree * node, int ** encList, int stringID)
{
  PrefixMatch * res = malloc(sizeof(PrefixMatch));
  res->pfxIndex = sfxIdx;
  res->lblIndex = node->incLblStart;
  res->next = NULL;
  res->last = node;

  showNode(node, 0);
  int * lblStr = encList[-node->stringID];

  int j; // ++(*pfx);
  printf("mPTEL: pfxLen=%i \n", pfxLen);
  for (j = 0; j < pfxLen - sfxIdx; ++j) { printf("%i ", *(*pfx + j)); }
  printf("\n");

  int pfx_exhausted = 0;
  int lbl_exhausted = 0;
  int mismatch = 0; 
  while (1) {
    printf(">>>>IN PM: sfxIdx=%i pfxIndex=%i/%i lblIndex=%i/%i\n", sfxIdx, res->pfxIndex, pfxLen, res->lblIndex, node->incLblEnd);
    // pfx_exhausted = (res->pfxIndex > pfxLen - sfxIdx);
    lbl_exhausted = (res->lblIndex > node->incLblEnd);
    res->result = (lbl_exhausted << 2 | pfx_exhausted << 1 | mismatch);
    if (4 == (6 & res->result)) {     
      visitNode(&node, stringID);  
      node = findEdgeByFirstChar(node, **pfx);
      if (NULL != node) {
        if (NULL != res->next) { res->last = res->next; }
        res->next = node;
        res->result &= 3;
        res->lblIndex = node->incLblStart;
        lblStr = encList[-node->stringID];
        printf("New Node:\n");
        showNode(node, 0);
      }
    }

    if (7 & res->result) { break; }

    printf("PM::%i <=> %i\n", **pfx, lblStr[res->lblIndex]);
    if (mismatch = (**pfx != lblStr[res->lblIndex])) {
      printf("PM::Mismatch %i (%i) <=> %i (%i)\n", **pfx, res->pfxIndex, lblStr[res->lblIndex], res->lblIndex);
      break;
    } else {
      ++(*pfx); ++res->pfxIndex; ++res->lblIndex; 
    }
  }
  res->result = (lbl_exhausted << 2 | pfx_exhausted << 1 | mismatch);
  printf("PM: pfxIx=%i lblIx=%i flags=%i pfx=%i \n", res->pfxIndex, res->lblIndex, res->result, **pfx);
  return res;
}



void setStringDepth(SFXTree * node, SFXTree * parent, int start, int end, int slen)
{
  int pdepth = 0;
  if (NULL != parent) { pdepth = parent->strDepth; }

  if ((slen == start) && (slen == end)) { 
    node->strDepth = pdepth;
    printf("Setting strDepth to parent depth: %i\n", pdepth); 
  } else {
    node->strDepth = pdepth + (end - start + 1); 
    printf("Setting strDepth: %i + (%i - %i + 1) = %i\n", pdepth, end, start, node->strDepth);
  }

  return ;
}







void insertStringNaive(SFXTree ** st, int stringID, int slen, int ** encList, int verbose)
{

  //char * cp = string;
  //int slen = strlen(string);
  //encList[-stringID] = encodeString(string, stringID); 
  int * enc_p = encList[-stringID], * ep;
  int i = 0, cur_start;
  //printf("Now inserting string %i \"%s(%i)\" (len=%i)...\nEncoded:\n", stringID, string, stringID, slen + 1);
  //showEncodedString(encList[-stringID], slen);
  
  SFXTree ** stp = st;
  SFXTree * next, * last, * child, * leaf;

  PrefixMatch * pm = NULL;

  // case 1: tree is empty
  // => add one new node with suffix S$ (=complete string)
  if (NULL == (*stp)->children) {
    printf("Case 1: EMPTY TREE - %s(%i)\n", "<string>", stringID);
    child = createNode(0, slen, stringID, i, encList[-stringID][i]);
    addChild(stp, child);
    i = 1; ++enc_p; 
    visitNode(&child, stringID);
    child->strDepth = slen; 
  }
  
  visitNode(stp, stringID);
  while (i < slen) {
    if (verbose) {                                                                                       
      printf("== TREE ==========================================================================:\n");
      showTree(*st, 0);
      printf("== TREE ==========================================================================:\n"); 
    }                                                                                                    

    cur_start = i; last = *stp; ep = enc_p;    
    while (1) {
      printf("Suffix: %i, CS=%i, EP is now %i (%i)\n", i, cur_start, *ep, *ep); 
      next = findEdgeByFirstChar(last, *ep);                                     
   
      if (NULL == next) {
        // case 2a: there is no such edge                            
        // => add a new leaf edge to the current node
        printf("Case 2a: %s(%i)\n", "<string>", stringID);
        next = createNode(i, slen, stringID, i, *ep);        
        visitNode(&next, stringID);
        //next->strDepth = last->strDepth + (slen - i);
        setStringDepth(next, last, i, slen, slen); 
        addChild(&last, next);
        break;
      } else {
        printf("NEXT: "); showNode(next, 0);
        // case 2b: there is an edge with the desired label           
        // => follow that edge until 
        if (NULL != pm) { free(pm); } 
        pm = matchPrefixToEdgeLabel(&ep, slen + 1, i, next, encList, stringID); 

        if ((6 == (6 & pm->result)) || (2 == (2 & pm->result))) {
          printf("This should not happen anymore: pm->result is %i. Aborting.\n", pm->result);
          exit(1);
        } else if (4 == (4 & pm->result)) {
          // this is again a case 2a: ultimately, the search for the suffix ended
          // at a node where no outgoing edge with the required label could be found
          // => add one new leaf edge to that node 
          printf("Case 2ai: %s(%i)\n", "<string>", stringID);
          if (NULL != pm->next) {
            last = pm->last; 
            next = pm->next; 
          }

          printf("LEAF: ");
          leaf = createNode(pm->pfxIndex, slen, stringID, i, encList[-stringID][pm->pfxIndex]);
          // leaf->strDepth = next->strDepth + (slen - pm->pfxIndex);
          setStringDepth(leaf, next, pm->pfxIndex, slen, slen);
          visitNode(&leaf, stringID);
          addChild(&next, leaf);
          break;
 
        } else if (1 == pm->result) {

            // case 2biv: mismatch occurred
            // => add a new leaf edge between "last" and "next" nodes
            printf("Case 2biv: %s(%i)\n", "<string>", stringID);
            printf("CS=%i LBLIX=%i PFXIX=%i\n", cur_start, pm->lblIndex, pm->pfxIndex);
            if (NULL != pm->next) {
              last = pm->last; 
              next = pm->next; 
            }

            printf("LEAF: ");
            leaf = createNode(pm->pfxIndex, slen, stringID, i, encList[-stringID][pm->pfxIndex]);
            printf("CHILD: ");
            child = createNode(next->incLblStart, pm->lblIndex - 1, next->stringID, next->sfxID, next->incFirstChar);
            child->nVisitingStrings = next->nVisitingStrings;
            child->lastVisitor = next->lastVisitor;
            addChild(&child, next);
            addChild(&child, leaf);
            setChildByFirstChar(last, child, next->incFirstChar);
            
            // child->strDepth = last->strDepth + (slen - next->incLblStart);
            // leaf->strDepth = child->strDepth + (slen - pm->pfxIndex);
            setStringDepth(child, last, next->incLblStart, pm->lblIndex - 1, slen);
            setStringDepth(leaf, child, pm->pfxIndex, slen, slen);
            
            visitNode(&leaf, stringID);
            visitNode(&child, stringID);
           
            printf("Changing "); showNode(next, 0);
            next->incLblStart = child->incLblEnd + 1; 
            next->incFirstChar = encList[-next->stringID][next->incLblStart];
            printf("To "); showNode(next, 0);
            break;            
   
        }
   
      }

    }

    ++i; ++enc_p;
    // printf("i: %i cp: %c enc_p: %i (%i)\n", i, cp, *enc_p, *enc_p); // DEBUG ONLY!
  }
  if (NULL != pm) { free(pm); }

}

void findCommonSubstrings(SFXTreeWrapper * sfx, int minlen, int k, int estReadLen)
{


  int i;
  SFXStack * stack = malloc(sizeof(SFXStack));
  initStack(stack, estReadLen);
  SFXTree * child;

  push(stack, sfx->tree);
  while (!stackIsEmpty(stack)) {
    SFXTree * node = pop(stack);
    printf("fCS-DFS: "); showNode(node, 0);

    int matches_k = (node->nVisitingStrings >= k);

    if (matches_k && (node->strDepth >= minlen)) {
      fprintf(stderr, "(k=%i >= %i)-common substring:\n", node->nVisitingStrings, k);
      char * ckSubstring = malloc(sizeof(char) * (node->strDepth + 1));
      char * cp = ckSubstring;

      for (i = node->incLblEnd - node->strDepth + 1; i <= node->incLblEnd; ++i, ++cp) {
        // fprintf(stderr, ">>i=%i\n", i);
        if (sfx->encodedStrings[-node->stringID][i] < 0) { break; }
        //fprintf(stderr, ">%i ", sfx->encodedStrings[-node->stringID][i]);
        *cp = sfx->encodedStrings[-node->stringID][i];
      }
      printf("\n");
      *cp = 0;
      fprintf(stderr, "=> %s\n", ckSubstring);
     
      free(ckSubstring);
    }

    // if node has children and the substring it is representing
    // is shared among at least k strings
    // then we have to check deeper down in the tree
    if (NULL != node->children && matches_k) {     
      SFXTree * child = *node->children;
      for (i = 0; i < node->lastChild; ++i) {
        printf(">%i\n", i);
        child = *(node->children + i);
        // ignore leaf-children that have only a terminator on their incoming edge
        if ((NULL != child) && (0 <= child->incFirstChar)) {       
          push(stack, child);
        }
      }
    }
  }
  
  
  //memset(stack, 0, sizeof(SFXTree) * stack->maxSize);  // if we don't set that space to 0, we might overwrite the stored pointers, no? 
  tearDownStack(stack);
  //free(stack); // still leaking... why?
  return ;

}

/*
int main(int argc, char ** argv) 
{
  //int ** encodedStringList = malloc(sizeof(int*) * 10);
  char * string1 = malloc(10 * sizeof(char));
  char * string2 = malloc(10 * sizeof(char));
  char * string3 = malloc(10 * sizeof(char));
 
  SFXTreeWrapper * sfx = initWrapper();
  
#if 0 
  strncpy(string1, "axabx\0", 6);
  strncpy(string2, "axabcx\0", 7);
  strncpy(string3, "axabcy\0", 7);
#elif 0
  strncpy(string1, "AA\0", 3);
  strncpy(string2, "AA\0", 3);
#else
  strncpy(string1, "TACCCAATG\0", 10);
  strncpy(string2, "ATACCGAAT\0", 10);
  strncpy(string3, "CGAATAATC\0", 10);
#endif
  
  


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
  
  return 0;



#if 0
  SFXTree * tree = initTree();

  insertStringNaive(&tree, string1, strlen(string1), -1, encodedStringList, !BE_QUIET);
  printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
  showTree(tree, 0);

  insertStringNaive(&tree, string2, strlen(string2), -2, encodedStringList, !BE_QUIET);
  printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
  showTree(tree, 0);

#if 0
  insertStringNaive(&tree, string3, strlen(string3), -3, encodedStringList, !BE_QUIET);
  printf("////////////////////////////////////////////////////////////////////////////////////////////7\n");
  showTree(tree, 0);
 #endif

  tearDown(tree);
  return 0;
#endif
}



int main2(int argc, char ** argv)
{
  
  char * str = malloc(11 * sizeof(char));
  strncpy(str, "0123456789", 10);
  char * cp = str;
  while (*cp) { printf("%i\n", *cp); ++cp; }


  int * ints = malloc(3 * sizeof(int)), * ints2 = malloc(3 * sizeof(int));
  int src[] = {1,1,2};
  memcpy(ints, src, 3*sizeof(int));
  src[1] = 30;
  src[0] = 30;
  memcpy(ints2, src, 3*sizeof(int));
  int * ip = ints;
  while (*ip) { printf("%i\n", *ip); ++ip; }
  
  int cmp = memcmp(ints, ints2, 3 * sizeof(int));
  printf("CMP: %i\n", cmp);

  return 0;
}
*/
