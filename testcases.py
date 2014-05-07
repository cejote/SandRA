#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import random

import suffix_tree as STREE
import seq


"""
@version: 2014-05-06
 
@summary:
ø create artificial read data (FW and RV)
ø append "pseudo adapter sequences" at 5' end
o introduce error with given rate
"""




def genadapter(length):
    """
    @summary: returns random adapter-like sequence of given length 
    """
    return (''.join(random.choice("ATCG") for i in range(length)))
    


def introduce_errors(seq, errorrate):
    """
    @TODO: not considered yet
    """
    return seq 


def main(argv):


    adaptercount=2#0
    adapterlen=30
    errorrate = 0.05

    readcount = 20
    readlen = 100
    minreadlen=40
    readlendelta=20

    adapters=[]


    ## init list of adapters    
    for i in xrange(adaptercount):
        adapters.append(genadapter(adapterlen).lower())
    print 'LEN', len(adapters), adapters

    ## produce reads 
    geneseq=seq.geneseq.lower()

    treeseqs = []
    for read in xrange(readcount):
        readstartpos = random.randint(0, readlen-minreadlen)
        lengthvar=random.randint(-readlendelta/2, readlendelta/2)

        adapt=random.choice(adapters)
        
        
        adaptstart=random.randint(0, len(adapt)/5)
        adaptend=random.randint(len(adapt)-(len(adapt)/5), len(adapt))


        #print adaptstart, adaptend
        #continue

        #adaptseq=adapt[adaptstart:adaptend]
        adaptseq=adapt
        readseq=geneseq[max(readstartpos,0):max(readstartpos,0)+readlen+lengthvar]
        
        treeseqs.append((adaptseq + readseq, adaptseq, adaptstart, adaptend))
        #print "@Seq_%s_%i"% (adaptseq, len(adaptseq))
        print  adaptseq + readseq
        #print "+"
        #print "I"*len(readseq)

        
    gst = STREE.GeneralisedSuffixTree([x[0].lower() for x in treeseqs])
    for hit in gst.ksharedSubstrings(minimumLength=25, k=6):
        #if len(hit) > 20:
        if hit[0][1] < 20:            
            print treeseqs[hit[0][0]][0][hit[0][1]:hit[0][2]], len(hit)
    return None
        


if __name__ == '__main__':
    main(sys.argv[1:])

