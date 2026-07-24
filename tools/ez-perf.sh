#!/bin/bash

# ==============================
# Verify input parameters
# ==============================

if [[ ("x$1" == "x") || ("x$3" != "x") ]]; then
    echo "Usage: $(basename $0) <Enzo-E output file> [ output directory ]"
    exit 1
fi

# ==============================
# Initialize path and file variables
#==============================

# Get absolute path to performance directory
file=$0
while [[ -L $file ]]; do
    file=`readlink $file`
done
topdir=$(dirname $file)
#topdir="$PWD/$topdir"

bindir="`dirname $0`"
if [ "$bindir" == "${bindir#/}" ]; then
    bindir="$PWD/$bindir"
fi
echo "bindir = $bindir"

# Get input file $input
input_raw="$PWD/$1"

# Get output directory $outdir (EZPerf by default)
outdir="$2"
if [ "x$outdir" == "x" ]; then
    outdir="EZPerf"
fi

# ==============================
# Create trace files if traces available
# ==============================

if compgen -G "PLOG.*" > /dev/null ; then
    cat PLOG.* > PLOG
    parse-trace.sh
    for cycle in Cycle-*; do
        cd $cycle
        gnuplot $bindir/EZPerf/plot-trace.gnu
        cd ..
    done
fi

# ==============================
# Create the output directory
# ==============================

if [[ -e "$outdir" ]]; then
    echo "Directory $outdir exists: skipping..."
    exit 1
fi
mkdir $outdir

echo "cp $topdir/cello.css $outdir"
cp $topdir/cello.css $outdir
cd $outdir

input="input-clean.data"
grep -v WARNING $input_raw > $input

BALANCE_EFF=`   awk '/perf:balance eff-/  {print $(NF-1)}' $input | sort | uniq`
BALANCE_MAX=`   awk '/perf:balance max-/  {print $(NF-1)}' $input | sort | uniq`
MEMORY=`        awk '/perf:region cycle / {print $(NF-1)}' $input | sort | uniq`
MESH=`          awk '/perf:mesh /         {print $(NF-1)}' $input | sort | uniq`
REDSHIFT=`      awk '/ redshift / {print $(NF-1)}' $input | sort | uniq`
REGION_ADAPT=`  awk '/perf:region adapt/  {print $(NF-2)}' $input | sort | uniq`
REGION_METHOD=` awk '/perf:region method/ {print $(NF-2)}' $input | sort | uniq`
REGION_REDUCE=` awk '/perf:region reduce/ {print $(NF-2)}' $input | sort | uniq`
REGION_REFRESH=`awk '/perf:region refresh/{print $(NF-2)}' $input | sort | uniq`
REGION_SMP=`    awk '/perf:region smp/    {print $(NF-2)}' $input | sort | uniq`
REGION_SOLVER=` awk '/perf:region solver/ {print $(NF-2)}' $input | sort | uniq`
REFRESH=`       awk '/perf:refresh / {print $(NF-1)}' $input | sort | uniq`
COUNTER=`       awk '/perf:counter / {print $(NF-1)}' $input | sort | uniq`

num_procs=`awk '/CkNumPes/  {print $5}' $input`
num_nodes=`awk '/CkNumNodes/{print $5}' $input`

# ==============================
# Generate data files
# ==============================

if [[ ! -e "cycle.data" ]]; then
    awk '/Simulation cycle /{if ($NF==0) {t0=$2}; print $NF,($2-t0)}' $input > cycle.data
fi


for adapt in $REGION_ADAPT; do
    if [[ ! -e "$adapt.data" ]]; then
        echo "Generating $adapt.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$adapt"' /{print c,$NF/'"$num_procs"'}' $input > $adapt.data
    fi
done

for refresh in $REGION_REFRESH; do
    if [[ ! -e "$refresh.data" ]]; then
        echo "Generating $refresh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$refresh"' /{print c,$NF/'"$num_procs"'}' $input > $refresh.data
    fi
done

for refresh in $REFRESH; do
    if [[ ! -e "$refresh.data" ]]; then
        echo "Generating msg-bytes-$refresh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:refresh refresh-bytes '"$refresh"' /{print c,$NF}' $input > msg-bytes_$refresh.data
        echo "Generating msg-count-$refresh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:refresh refresh-count '"$refresh"' /{print c,$NF}' $input > msg-count_$refresh.data
        echo "Generating msg-sizes-$refresh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:refresh refresh-bytes '"$refresh"' /{rb=$NF}; /perf:refresh refresh-count '"$refresh"' /{rc=$NF}; /------------/{if (rc != "") print c,rb/rc}' $input > msg-sizes_$refresh.data
        echo "Generating msg-sizes-cycle-$refresh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:refresh refresh-bytes '"$refresh"' /{rb0=rb; rb=$NF}; /perf:refresh refresh-count '"$refresh"' /{rc0=rc; rc=$NF}; /------------/{if ((rb-rb0 > 0) && (rc-rc0 > 0)) print c,(rb-rb0)/(rc-rc0)}' $input > msg-sizes-cycle_$refresh.data
    fi
done

for counter in $COUNTER; do
     echo "Generating counter-$counter.data"
     awk '/Simulation cycle /{c=$NF}; /perf:counter '"$counter"' /{print c,$NF}' $input > counter-$counter.data
done

for reduce in $REGION_REDUCE; do
    if [[ ! -e "$reduce.data" ]]; then
        echo "Generating $reduce.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$reduce"' /{print c,$NF/'"$num_procs"'}' $input > $reduce.data
    fi
done

for redshift in $REDSHIFT; do
    if [[ ! -e "$redshift.data" ]]; then
        echo "Generating $redshift.data"
        awk '/Simulation cycle /{c=$NF}; /Simulation redshift/{print c,$NF}' $input > $redshift.data
    fi
done

for smp in $REGION_SMP; do
    if [[ ! -e "$smp.data" ]]; then
        echo "Generating $smp.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$smp"' /{print c,$NF/'"$num_procs"'}' $input > $smp.data
    fi
done

for method in $REGION_METHOD; do
    if [[ ! -e "$method.data" ]]; then
        echo "Generating $method.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$method"' /{print c,$NF/'"$num_procs"'}' $input > $method.data
    fi
done

for solver in $REGION_SOLVER; do
    if [[ ! -e "$solver.data" ]]; then
        echo "Generating $solver.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region '"$solver"' /{print c,$NF/'"$num_procs"'}' $input > $solver.data
    fi
done

for memory in $MEMORY; do
    if [[ ! -e "$memory.data" ]]; then
        echo "Generating $memory.data"
        awk '/Simulation cycle /{c=$NF}; /perf:region cycle '"$memory"' /{print c,$NF/'"$num_procs"'}' $input > $memory.data
    fi
done

for mesh in $MESH; do
    if [[ ! -e "$mesh.data" ]]; then
        echo "Generating $mesh.data"
        awk '/Simulation cycle /{c=$NF}; /perf:mesh '"$mesh"' /{print c,$NF}' $input > mesh_$mesh.data
    fi
done

for balance in $BALANCE_EFF; do
    if [[ ! -e "$balance.data" ]]; then
        echo "Generating $balance.data"
        awk '/Simulation cycle /{c=$NF}; /perf:balance '"$balance"' /{print c,$NF}' $input > balance_$balance.data
    fi
done
for balance in $BALANCE_MAX; do
    if [[ ! -e "$balance.data" ]]; then
        echo "Generating $balance.data"
        awk '/Simulation cycle /{c=$NF}; /perf:balance '"$balance"' /{print c,$NF}' $input > balance_$balance.data
    fi
done

# ==============================
# Generate plots from data files
# ==============================

echo "Generating plots..."
$bindir/EZPerf/_plot-perf.py

echo "file://$PWD/index.html"
