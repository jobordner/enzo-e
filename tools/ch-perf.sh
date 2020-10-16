#!/bin/bash

file="$1"
out_accum="plot-accum.gnu"
out_cycle="plot-cycle.gnu"
if [[ ("x$1" == "x") || ("x$3" != "x") ]]; then
    echo "Usage: $0 <file_name> [plot_name]"
    exit 1
fi

if [[ "x$2" != "x" ]]; then
    filename="$2"
else
    filename=`basename $file`
fi

filename=${filename//\_/\\_}

METHOD=`awk '/perf_method_/{print $4}' $file |sort |uniq`
SOLVER=`awk '/perf_solver_/{print $4}' $file |sort |uniq`

rm -f $out_accum
rm -f $out_cycle
echo "set terminal png size 1024, 800" >> $out_accum
echo "set terminal png size 1024, 800" >> $out_cycle
echo "set output 'plot-accum.png'" >> $out_accum
echo "set output 'plot-cycle.png'" >> $out_cycle

echo "set title '${filename}: cycle versus time (accumulated)'" >> $out_accum
echo "set title '${filename}: cycle versus time (per cycle)'" >> $out_cycle
echo "set key Left bottom right" >> $out_accum
echo "set key Left bottom right" >> $out_cycle
echo "set grid" >> $out_accum
echo "set grid" >> $out_cycle
echo "set log y" >> $out_accum
echo "set log y" >> $out_cycle
echo "set ylabel 'seconds'" >> $out_accum
echo "set ylabel 'seconds'" >> $out_accum
echo "set xlabel 'cycles'" >> $out_accum
echo "set xlabel 'cycles'" >> $out_accum
printf "plot " >> $out_accum
printf "plot " >> $out_cycle

echo $simulation
#using="using (\$1):(\$2/1000000)" # vs time
using="using (\$2/1000000)" # vs cycle
simulation="perf_simulation"
simulation_title=${simulation//perf\_/}
simulation_title=${simulation_title//\_/\\_}
awk '/'" $simulation "'/{print $2, $6}' $file > $simulation-accum.data
awk '/'" $simulation "'/{print $2, $6-c; c=$6}' $file > $simulation-cycle.data
echo "\\" >> $out_accum
echo "\\" >> $out_cycle
printf "'$simulation-accum.data' $using title '$simulation_title' w lp" >> $out_accum
printf "'$simulation-cycle.data' $using title '$simulation_title' w lp" >> $out_cycle

for method in $METHOD; do
    echo $method
    method_title=${method//perf\_/}
    method_title=${method_title//\_/\\_}
    title="title '$method_title'"
    awk '/'" $method "'/{print $2, $6}' $file > $method-accum.data
    awk '/'" $method "'/{print $2, $6-c; c=$6}' $file > $method-cycle.data
    echo ", \\" >> $out_accum
    echo ", \\" >> $out_cycle
    printf "'$method-accum.data' $using $title w lp" >> $out_accum
    printf "'$method-cycle.data' $using $title w lp" >> $out_cycle
    seperator=", "
done

for solver in $SOLVER; do
    echo $solver
    solver_title=${solver//perf\_/}
    solver_title=${solver_title//\_/\\_}
    awk '/'" $solver "'/{print $2, $6}' $file > $solver-accum.data
    awk '/'" $solver "'/{print $2, $6-c; c=$6}' $file > $solver-cycle.data
    echo ", \\" >> $out_accum
    echo ", \\" >> $out_cycle
    printf "'$solver-accum.data' $using title '$solver_title' w lp" >> $out_accum
    printf "'$solver-cycle.data' $using title '$solver_title' w lp" >> $out_cycle
    seperator=", "
done
echo >> $out_accum
echo >> $out_cycle

rm -f plot-accum.png
gnuplot plot-accum.gnu
rm -f plot-cycle.png
gnuplot plot-cycle.gnu
