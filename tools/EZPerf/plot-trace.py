import matplotlib.pyplot as plt
from numpy import *
import numpy as np
import glob
import subprocess
from matplotlib.patches import Rectangle, Circle
# Create a figure and axis


awk_out=subprocess.run(["awk","/\[/{print $2}","PLOG"],
                       capture_output=True,check=True)
sort_out=subprocess.run(["sort"], input=awk_out.stdout,
                      capture_output=True,check=True)
uniq_out=subprocess.run(["uniq"], input=sort_out.stdout,
                      capture_output=True,check=True)
print (uniq_out)
exit(1)
fig, ax = plt.subplots(figsize=(16,4))
xmax_g=-10000
xmax_g=-10000
xmin_g=100000
ymin_g=10000

# BLOCK
#pad=0.0
#lw=1
# PROC
pad=0.25
lw=1

c=['m','g','c','C1','y','b','r','k']
index=0
ax.grid()
ax.set_axisbelow(True)
for filename in sort(glob.glob("401-M*.data")):

    data = loadtxt(filename,dtype=float)
    block=data[:,0]
    proc=data[:,1]
    tstart=data[:,2]
    tstop=data[:,3]

    # Draw some shapes for demonstration
    xmin=min(tstart)
    xmax=max(tstop)
    ymin=min(proc)
    ymax=max(proc)
    xmin_g=min([xmin_g,xmin])
    xmax_g=max([xmax_g,xmax])
    ymin_g=min([ymin_g,ymin])
    ymax_g=max([ymin_g,ymax])
    shape=tstart.shape
    dx=(xmax-xmin)/1000
    for i in range(shape[0]):
        ax.add_patch(Rectangle((tstart[i],proc[i]-pad), max([dx,tstop[i]-tstart[i]]),1.0-2*pad, fc=c[index],
                     edgecolor=c[index],
                     linewidth=lw))
    index = index + 1
plt.ylim(ymin_g-1,ymax_g+1)
plt.xlim(xmin_g,xmax_g)

plt.show()
fig.savefig('bar.png',format='png')

