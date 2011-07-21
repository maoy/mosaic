#!/usr/bin/python
import random,sys

def topology(n, degree):
    """ n-node network, and each node has exactly degree links
    """
    for i in range(n):
        dest = range(0,i)+range(i+1,n)
        random.shuffle(dest)
        for j in range(degree):
            #print 'link("n%d","n%d",1);' % (i,dest[j])
            print "link,n%d,n%d,1" % (i,dest[j])

if __name__=="__main__":
    if (len(sys.argv)!=3):
        print "usage: link_generator.py node degree"
    else:
        topology(int(sys.argv[1]),int(sys.argv[2]) )

