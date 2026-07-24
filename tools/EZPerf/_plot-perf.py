#!/usr/bin/python

from numpy import *
import numpy as np
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.pyplot import figure
import glob
import os.path
import string

#--------------------------------------------------
def plot_open(plt,title,label_x,label_y):
    print ("Plotting",title)

    plt.clf()

    plt.axes([0.1,0.1,0.8,0.8])
    plt.figtext(0.5,0.95,title,fontsize=18, ha='center')
    # plt.figtext(0.5,0.915, 'N7 P(1,*,1) net-gcc', fontsize=12, ha='center')
    plt.xlabel(label_x)
    plt.ylabel(label_y)
    plt.grid(True)

#--------------------------------------------------
def plot_total(plt,filename,label,type='total',scale=1e-6):
    data =  loadtxt(filename,dtype=float)
    total_x = data[:,0]
    total_y = data[:,1]
    if (type == 'cycle'):
        n=len(total_x)
        total_x = total_x[1:n-1]
        total_y = total_y[2:n] - total_y[1:n-1]
    plt.plot(total_x,scale*total_y, label=label, color=c[0],marker=m[0], ls=l, lw=w)
    return [total_x, total_y]

def plot_file (plt,i,file,type,scale):
    data =  loadtxt(file,dtype=float)
    file_x = data[:,0]
    file_y = data[:,1]
#    total =  loadtxt('cycle.data',dtype=float)
#    total_x = data[:,0]
#    total_y = data[:,1]
    if (type == 'cycle'):
        n=len(file_x)
        file_x = file_x[1:n-1]
        file_y = file_y[2:n] - file_y[1:n-1]
#        total_x = total_x[1:n-1]
#        total_y = total_y[2:n] - total_y[1:n-1]
    name=os.path.splitext(os.path.basename(file))[0]
    i_ = name.find("_")
    if (i_ > 0):
        temp=name[:i_+1]
        name = name.replace(temp,"")
 #   plt.plot(file_x,scale*file_y/total_y, label=name, color=c[i%7],marker=m[i%7], ls=l, lw=w)
    plt.plot(file_x,scale*file_y, label=name, color=c[i%7],marker=m[i%7], ls=l, lw=w)

#--------------------------------------------------
def plot_list(plt,file_list,type='total',scale=1e-6,sort=True):
    if (sort):
        # sort file names by reverse end time
        file_times = []
        for file in file_list:
            if (os.path.exists(file) and (not file.endswith(('_post.data')))):
                data =  loadtxt(file,dtype=float)
                if (len(data) > 2):
                    glob_x = data[:,0]
                    glob_y = data[:,1]
                    file_times.append([glob_y[len(glob_y)-1],file])
        i=1
        # plot by reverse end time
        for row in sorted(file_times,reverse=True):
            file = row[1]
            if (os.path.exists(file)):
                plot_file (plt,i,file,type,scale)
                i=i+1
    else:
        i=1
        # plot by reverse end time
        for file in sorted(file_list):
            plot_file (plt,i,file,type,scale)
            i=i+1
#--------------------------------------------------

def plot_write(name,html):
    print ("Writing '%s'" % (name))
    plt.savefig(('%s.pdf') % (name), format='pdf')
    plt.savefig(('%s.png') % (name), format='png')
    html_table_cell_image(html,name)

#====================================================================== 

def html_start():

    # Get directory name containing EZPerf directory
    t1=os.getcwd()
    i1 = t1.find("/EZPerf")
    print("i1 = ",i1)
    t2=t1[:i1]
    print("t2 = ",t2)
    i2=t2.rfind("/")
    print ("i2 = ",i2)
    run=t2[i2+1:]

    html=open('index.html', 'w')
    html.write('<HTML>\n')
    html.write('  <HEAD>\n')
    html.write('    <link href="../cello.css" rel="stylesheet" type="text/css">\n')
    html.write('  </head>\n')
    html.write('    <title>Enzo-E / Cello EZPerf: run.'+run+'</title>\n')
    html.write('    <body>\n')
    html.write('      <h1>Enzo-E / Cello EZPerf: run.'+run+'</h1>\n')
    return html

def html_stop(html):
    html.write('    </body>\n')
    html.write('</html>\n')

def html_table_start(html):
    html.write('      <table>\n')

def html_table_stop(html):
    html.write('      </table>\n')

def html_table_row_start(html):
    html.write('         <tr>\n')

def html_table_row_stop(html):
    html.write('         </tr>\n')

def html_table_row_next(html,index,max_rows):
    index = index + 1
    if (index > max_rows):
        index = 1
        html_table_row_stop(html)
        html_table_row_start(html)
    return index

def html_section_h1(html,name):
    html.write('<h1> '+name+' </h1>\n')
def html_section_h2(html,name):
    html.write('<h2> '+name+' </h2>\n')
def html_section_h3(html,name):
    html.write('<h3> '+name+' </h3>\n')

def html_table_cell_image(html,name):
    image=name+".png"
    html.write('            <td>\n')
    html.write('              <a href="' +image+'"><img width=480 src="'+image+'"></img></a>\n')
    if (Path("../../index-"+name+".html")).is_file():
        html.write('<center><a href="../../index-'+name+'.html">this plot all runs</a></center>')

def plot_time_total(plt,region_list,html):
    plot_open(plt,'cumulative times','cycle','time (s)');
    plot_total(plt,'cycle.data','cycle',scale=1.0)
    plot_list(plt,region_list)
    plt.legend(loc='upper left',ncols=3)
#    plt.yscale('log')
    plot_write('plot_time_total',html)

# ----------------------------------------------------------------------
def plot_time_cycle(plt,region_list,html):
    plot_open(plt,'per-cycle times','cycle','time (s)');
    plot_total(plt,'cycle.data','cycle',scale=1.0,type='cycle')
    plot_list(plt,region_list,type='cycle')
    plt.legend(loc='upper left',ncols=3)
    plot_write('plot_time_cycle',html)

# ----------------------------------------------------------------------

c=['b', 'g', 'r', 'c', 'm', 'y', 'k']
m=['>', '^', '<', 'v', 'o', '+', 'x', 's' ]
m=['none', 'none', 'none', 'none', 'none', 'none', 'none', 'none']
l='-'
w=1

figure(figsize=(8,6), dpi=100)

html=html_start()


# ----------------------------------------------------------------------
region_list = []
if os.path.exists('method.data'):
    region_list.append('method.data')
if os.path.exists('solver.data'):
    region_list.append('solver.data')
if os.path.exists('refresh.data'):
    region_list.append('refresh.data')
if os.path.exists('adapt.data'):
    region_list.append('adapt.data')
if os.path.exists('reduce.data'):
    region_list.append('reduce.data')
if os.path.exists('smp.data'):
    region_list.append('smp.data')

# ======================================================================
# ROW 2: MEMORY, BLOCKS, BALANCE
# ======================================================================

html_section_h2(html,"Memory, blocks, and load balance")
html_table_start(html)
html_table_row_start(html)

# ----------------------------------------------------------------------
plot_open(plt,'memory usage','cycle','Mbytes');
plot_list(plt,glob.glob('memory_*data'))
plt.legend(loc='upper left',ncols=3)
ym,yp = plt.ylim()
plt.ylim(0,yp)
plot_write('plot_memory',html)
# ----------------------------------------------------------------------
if os.path.exists('mesh_total-blocks.data'):
    plot_open(plt,'blocks per level','cycle','blocks');
    plot_total(plt,'mesh_total-blocks.data','mesh_blocks-total',scale=1.0)
    plot_list(plt,glob.glob('mesh_blocks*.data'),scale=1.0,sort=False)
    plt.legend(loc='upper left',ncols=1)
    plot_write('plot_mesh',html)
# ----------------------------------------------------------------------
plot_open(plt,'load-balance efficiency','cycle','efficiency');
plot_list(plt,glob.glob('balance_eff-*data'),scale=1.0)
plt.legend(loc='lower left',ncols=3)
plt.ylim(0,1)
plot_write('plot_balance_eff',html)
# ----------------------------------------------------------------------


html_table_row_stop(html)
html_table_stop(html)

# ======================================================================
# ROW 3: TOTAL TIMES
# ======================================================================

html_section_h2(html,"Absolute cumulative times")

html_table_start(html)
html_table_row_start(html)

plot_time_total(plt,region_list,html)

index = 2
max_index = 3
# ----------------------------------------------------------------------
if os.path.exists('method.data'):
    plot_open(plt,'cumulative method time','cycle','time (s)');
    [method_x_total, method_y_total] = plot_total(plt,'method.data','method')
    plot_list(plt,glob.glob('method_*data'))
    plt.legend(loc='upper left',ncols=3)
    #plt.yscale('log')
    plot_write('plot_method_total',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('solver.data'):
    plot_open(plt,'cumulative solver time','cycle','time (s)');
    [solver_x_total, solver_y_total] = plot_total(plt,'solver.data','solver')
    print (glob.glob('solver_*data'))
    plot_list(plt,glob.glob('solver_*data'))
    plt.legend(loc='upper left',ncols=3)
    #plt.yscale('log')
    plot_write('plot_solver_total',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('adapt.data'):
    plot_open(plt,'cumulative adapt time','cycle','time (s)');
    [adapt_x_total, adapt_y_total] = plot_total(plt,'adapt.data','adapt')
    plot_list(plt,glob.glob('adapt_*data'))
    plt.legend(loc='upper left',ncols=3)
    #plt.yscale('log')
    plot_write('plot_adapt_total',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('refresh.data'):
    plot_open(plt,'cumulative refresh time','cycle','time (s)');
    [refresh_x_total, refresh_y_total] = plot_total(plt,'refresh.data','refresh')
    plot_list(plt,glob.glob('refresh_*data'))
    plt.legend(loc='upper left',ncols=2)
    #plt.yscale('log')
    plot_write('plot_refresh_total',html)
    index = html_table_row_next(html,index,max_index)

plot_open(plt,'cumulative refresh message counts','cycle','messages');
plot_list(plt,glob.glob('msg-count_*data'),scale=1.0)
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_count',html)
index = html_table_row_next(html,index,max_index)

plot_open(plt,'cumulative refresh message bytes','cycle','MBytes total');
plot_list(plt,glob.glob('msg-bytes_*data'))
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_bytes',html)
index = html_table_row_next(html,index,max_index)

plot_open(plt,'cumulative average message size','cycle','Average MBytes/message');
plot_list(plt,glob.glob('msg-sizes_*data'),scale=1.0)
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_sizes',html)
index = html_table_row_next(html,index,max_index)

# plot_open(plt,'cumulative undeleted Charm++ messages','cycle','messages');
# plot_list(plt,glob.glob('counter-*data'),scale=1.0)
# plt.legend(loc='upper left',ncols=2)
# #plt.yscale('log')
# plot_write('counter',html)
# index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('redshift.data'):
    plot_open(plt,'redshift','cycle','redshift');
    [redshift_x_total, redshift_y_total] = plot_total(plt,'redshift.data','redshift',scale=1.0)
    plot_list(plt,glob.glob('redshift_*data'))
    plt.legend(loc='upper left',ncols=3)
    #plt.yscale('log')
    plot_write('plot_redshift_total',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('smp.data'):
    plot_open(plt,'cumulative SMP time','cycle','time (s)');
    [smp_x_total, smp_y_total] = plot_total(plt,'smp.data','smp')
    plot_list(plt,glob.glob('smp_*data'))
    plt.legend(loc='upper left',ncols=3)
#    plt.yscale('log')
    plot_write('plot_smp_total',html)
    index = html_table_row_next(html,index,max_index)
# ----------------------------------------------------------------------

html_table_row_stop(html)
html_table_stop(html)

# ======================================================================
# ROW 4: CYCLE TIMES
# ======================================================================

html_section_h2(html,"Absolute per-cycle times")

html_table_start(html)
html_table_row_start(html)

plot_time_cycle(plt,region_list,html)

index = 2

# ----------------------------------------------------------------------
if os.path.exists('method.data'):
    plot_open(plt,'per-cycle method time','cycle','time (s)');
    [method_x_total, method_y_total] = plot_total(plt,'method.data','method',type='cycle')
    plot_list(plt,glob.glob('method_*data'),type='cycle')
    plt.legend(loc='upper left',ncols=3)
    plot_write('plot_method_cycle',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('solver.data'):
    plot_open(plt,'per-cycle solver time','cycle','time (s)');
    [solver_x_total, solver_y_total] = plot_total(plt,'solver.data','solver',type='cycle')
    plot_list(plt,glob.glob('solver_*data'),type='cycle')
    plt.legend(loc='upper left',ncols=3)
    plot_write('plot_solver_cycle',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------
if os.path.exists('adapt.data'):
    plot_open(plt,'per-cycle adapt time','cycle','time (s)');
    [adapt_x_total, adapt_y_total] = plot_total(plt,'adapt.data','adapt',type='cycle')
    plot_list(plt,glob.glob('adapt_*data'),type='cycle')
    plt.legend(loc='upper left',ncols=3)
    plot_write('plot_adapt_cycle',html)
    index = html_table_row_next(html,index,max_index)

# ----------------------------------------------------------------------

if os.path.exists('refresh.data'):
    plot_open(plt,'per-cycle refresh time','cycle','time (s)');
    [refresh_x_total, refresh_y_total] = plot_total(plt,'refresh.data','refresh',type='cycle')
    plot_list(plt,glob.glob('refresh_*data'),type='cycle')
    plt.legend(loc='upper left',ncols=2)
    plot_write('plot_refresh_cycle',html)
    index = html_table_row_next(html,index,max_index)

lsave=l
msave=m
ms=10
l='None'
m=['>', '^', '<', 'v', 'o', '+', 'x', 's' ]
plot_open(plt,'per-cycle refresh message counts','cycle','messages');
plot_list(plt,glob.glob('msg-count_*data'),scale=1.0,type='cycle')
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_count_cycle',html)
index = html_table_row_next(html,index,max_index)

plot_open(plt,'per-cycle refresh message bytes','cycle','MBytes total');
plot_list(plt,glob.glob('msg-bytes_*data'),type='cycle')
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_bytes_cycle',html)
index = html_table_row_next(html,index,max_index)

plot_open(plt,'per-cycle average message size','cycle','Average KBytes/message');
plot_list(plt,glob.glob('msg-sizes-cycle_*data'),scale=1e-3)
plt.legend(loc='upper left',ncols=2)
#plt.yscale('log')
plot_write('plot_msg_sizes_cycle',html)
index = html_table_row_next(html,index,max_index)

# plot_open(plt,'per-cycle undeleted Charm++ messages','cycle','messages');
# plot_list(plt,glob.glob('counter-*data'),scale=1.0,type='cycle')
# plt.legend(loc='upper left',ncols=2)
# plot_write('counter_cycle',html)
# index = html_table_row_next(html,index,max_index)

l=lsave
m=msave

# ----------------------------------------------------------------------
if os.path.exists('smp.data'):
    plot_open(plt,'per-cycle smp time','cycle','time (s)');
    [smp_x_total, smp_y_total] = plot_total(plt,'smp.data','smp',type='cycle')
    plot_list(plt,glob.glob('smp_*data'),type='cycle')
    plt.legend(loc='upper left',ncols=3)
    plot_write('plot_smp_cycle',html)
    index = html_table_row_next(html,index,max_index)
# ----------------------------------------------------------------------
html_table_row_stop(html)
html_table_stop(html)

# ======================================================================
# Include trace files if available

html_section_h2(html,"Method and refresh traces")
if os.path.exists('../PLOG'):
    html_table_start(html)
    for cycle_dir in glob.glob('../Cycle-*'):
        html_table_row_start(html)
        html_table_cell_image(html,cycle_dir+"/trace-method")
        html_table_cell_image(html,cycle_dir+"/trace-refresh")
        html_table_cell_image(html,cycle_dir+"/trace-method-sorted")
        html_table_cell_image(html,cycle_dir+"/trace-refresh-sorted")
        html_table_row_stop(html)
    html_table_stop(html)

# ======================================================================

html_table_stop(html)
html_stop(html)
