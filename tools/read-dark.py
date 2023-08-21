#!/bin/python3
import sys
import random
dark = ["particle_dark_x", "particle_dark_y", "particle_dark_z"]
f=open('dark.xyz','w')
import h5py
ratio=float(sys.argv[1])
for name in sys.argv[2:]:
    file = h5py.File(name,'r')
    print('Reading',name)
    for block in file.keys():
        n=file[block][dark[0]].size
        for i in range(n):
            if (random.random() < ratio):
                f.write ("%7f %7f %7f" %
                       (file[block][dark[0]][i],
                        file[block][dark[1]][i],
                        file[block][dark[2]][i]))
