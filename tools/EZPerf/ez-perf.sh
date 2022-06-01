#!/bin/bash

# Check that input parameters are as expected
if [[ ("x$1" == "x") || ("x$3" != "x") ]]; then
    echo "Usage: $(basename $0) <Enzo-E output file> [ output directory ]"
    exit 1
fi

# Get path $topdir to performance directory (even if $0 is a symbolic link)
file=$0
while [[ -L $file ]]; do
      file=`readlink $file`
done
topdir=$(dirname $file)

# Get input file

input="$PWD/$1"

# Get output directory if any (or "EZPerf" by default)

outdir="$2"
if [ "x$outdir" == "x" ]; then
    outdir="EZPerf"
fi

# Create the output directory if possible, else error
if [[ -e $outdir ]]; then
    if [[ -e $outdir.backup ]]; then
        echo "Deleting $outdir.backup!"
        rm -rf $outdir.backup
    fi
    mv $outdir $outdir.backup
fi
mkdir $outdir
cd $outdir

ADAPT=`awk '/perf:region adapt/{print $(NF-2)}' $input | sort | uniq`
REFRESH=`awk '/perf:region refresh/{print $(NF-2)}' $input | sort | uniq`
METHOD=`awk '/perf:region method/{print $(NF-2)}' $input | sort | uniq`
SOLVER=`awk '/perf:region solver/{print $(NF-2)}' $input | sort | uniq`
MEMORY=`awk '/perf:region cycle /{print $(NF-1)}' $input | sort | uniq`
BALANCE=`awk '/perf:balance /{print $(NF-2)}' $input | sort | uniq`
MESH=`awk '/perf:mesh /{print $(NF-1)}' $input | sort | uniq`

num_procs=`awk '/CkNumPes/  {print $5}' $input`
num_nodes=`awk '/CkNumNodes/{print $5}' $input`

echo "nodes $num_nodes procs $num_procs"
if [[ ! -e "cycle.data" ]]; then
    awk '/Simulation cycle/{if ($NF==0) {t0=$2}; print $NF,'"$num_procs"'*($2-t0)}' $input > cycle.data
fi


for adapt in $ADAPT; do
    if [[ ! -e "$adapt.data" ]]; then
           echo "Generating $adapt.data"
           awk '/Simulation cycle/{c=$NF}; /perf:region '"$adapt"' /{print c,$NF}' $input > $adapt.data
       fi
done

for refresh in $REFRESH; do
    if [[ ! -e "$refresh.data" ]]; then
           echo "Generating $refresh.data"
           awk '/Simulation cycle/{c=$NF}; /perf:region '"$refresh"' /{print c,$NF}' $input > $refresh.data
       fi
done

for method in $METHOD; do
    if [[ ! -e "$method.data" ]]; then
        echo "Generating $method.data"
        awk '/Simulation cycle/{c=$NF}; /perf:region '"$method"' /{print c,$NF}' $input > $method.data
    fi
done

for solver in $SOLVER; do
    if [[ ! -e "$solver.data" ]]; then
        echo "Generating $solver.data"
        awk '/Simulation cycle/{c=$NF}; /perf:region '"$solver"' /{print c,$NF}' $input > $solver.data
    fi
done

for memory in $MEMORY; do
    if [[ ! -e "$memory.data" ]]; then
        echo "Generating $memory.data"
        awk '/Simulation cycle/{c=$NF}; /perf:region cycle '"$memory"' /{print c,$NF}' $input > $memory.data
    fi
done
for mesh in $MESH; do
    if [[ ! -e "$mesh.data" ]]; then
        echo "Generating $mesh.data"
        awk '/Simulation cycle/{c=$NF}; /perf:mesh '"$mesh"' /{print c,$NF}' $input > $mesh.data
    fi
done

for balance in $BALANCE; do
    if [[ ! -e "$balance.data" ]]; then
           echo "Generating $balance.data"
           awk '/Simulation cycle/{c=$NF}; /perf:balance '"$balance"' /{print c,$(NF-1)}' $input > $balance.data
       fi
done

if [[ ! -e "plot-adapt.png" ]]; then
    echo "Generating plot-adapt.png"
    gnuplot $topdir/plot-adapt.gnu
fi
if [[ ! -e "plot-adapt-post.png" ]]; then
    echo "Generating plot-adapt-post.png"
    gnuplot $topdir/plot-adapt-post.gnu
fi

if [[ ! -e "plot-refresh.png" ]]; then
    echo "Generating plot-refresh.png"
    gnuplot $topdir/plot-refresh.gnu
fi
if [[ ! -e "plot-refresh-post.png" ]]; then
    echo "Generating plot-refresh-post.png"
    gnuplot $topdir/plot-refresh-post.gnu
fi

if [[ ! -e "plot-method.png" ]]; then
    echo "Generating plot-method.png"
    gnuplot $topdir/plot-method.gnu
fi
if [[ ! -e "plot-balance.png" ]]; then
    echo "Generating plot-balance.png"
    gnuplot $topdir/plot-balance.gnu
fi
if [[ ! -e "plot-solver.png" ]]; then
    echo "Generating plot-solver.png"
    gnuplot $topdir/plot-solver.gnu
fi
if [[ ! -e "plot-memory.png" ]]; then
    echo "Generating plot-memory.png"
    gnuplot $topdir/plot-memory.gnu
fi
if [[ ! -e "plot-mesh.png" ]]; then
    echo "Generating plot-mesh.png"
    gnuplot $topdir/plot-mesh.gnu
fi
if [[ ! -e "plot-perf.png" ]]; then
    echo "Generating plot-perf.png"
    gnuplot $topdir/plot-perf.gnu
fi

