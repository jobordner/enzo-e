#!/usr/bin/env python
import sys
import fileinput
import matplotlib
import matplotlib.pyplot as plt
import re
import numpy as np

def decode_block(block_name):
    prefix="B"
    lower=np.array([0,0,0])
    size=np.array([1,1,1])
    max_level = len(block_name) - 1
    factor = 1
    level = 0
    while level < max_level:
        size = 0.5*size
        d=block_name[level+1]
        n=ord(d)-ord('0')
        c = np.array([n&1,(n&2)>>1,(n&4)>>2])
        lower = lower + size*c
        level = level + 1
    upper = lower + size
    return [level,lower, upper]

#======================================================================

filename=""
if (len(sys.argv) > 1):
    arg=sys.argv[1]
    if (arg[len(arg)-4:len(arg)] == ".png"):
        filename = arg
        print (filename)
        sys.argv.pop(0)

fig = plt.figure()
ax = fig.add_subplot(111)
xmin=10000; xmax=-10000
ymin=10000; ymax=-10000
colormap = ['red','orange','green','blue','magenta','cyan']
lines = []
for line in fileinput.input():
      lines.append(line)
max_level = 10

for plotlevel in range(0,max_level):
      for line in lines:
          line_list = line.split();
          for word in line_list:
              if (re.search(r'^Z[0-7]+',word)):

                  [level,lower, upper] = decode_block(word)

                  if (level == plotlevel):
                      nc = len(colormap)
                      r = matplotlib.patches.Rectangle ( (lower[0],lower[1]), 
                                                         (upper[0]-lower[0]), 
                                                         (upper[1]-lower[1]), 
                                                         fill=True,
                                                         edgecolor=colormap[level % nc])
                      xmin = min(xmin,lower[0])
                      ymin = min(ymin,lower[1])
                      xmax = max(xmax,upper[0])
                      ymax = max(ymax,upper[1])
                      ax.add_patch(r)

if (True):
    xsize = xmax - xmin
    ysize = ymax - ymin
    dsize = abs(xsize - ysize)

    # 1:1 aspect ratio
    if (xsize > ysize):
        ymin -= 0.5*dsize
        ymax += 0.5*dsize
        ysize = ymax - ymin
    elif (ysize > xsize):
        xmin -= 0.5*dsize
        xmax += 0.5*dsize
        xsize = xmax - xmin

    # add border
    r=0.2*xsize
    xmin -= r
    ymin -= r
    xmax += r
    ymax += r

    #plt.axes().set_aspect('equal','datalim')
    xmin=-0.2
    ymin=-0.2
    xmax=1.2
    ymax=1.2
    plt.xlim([xmin,xmax])
    plt.ylim([ymin,ymax])
    if (filename == ""):
        plt.show()
    else:
        matplotlib.pyplot.savefig(filename, dpi=None, facecolor='w', edgecolor='w', orientation='portrait', format=None, transparent=False, bbox_inches=None, pad_inches=0.1, metadata=None)
