#!/bin/bash

# Creat PLOG
cat PLOG.* >PLOG

# Initialize variables
CYCLES=`awk '/\[/{print $2}' PLOG | sort | uniq`
REGIONS=`awk '/\[/{print $5}' PLOG | sort | uniq`
  METHOD_NUM=(`awk '/#M/{print $2}' PLOG.0`)
 METHOD_NAME=(`awk '/#M/{print $3}' PLOG.0`)
  SOLVER_NUM=(`awk '/#S/{print $2}' PLOG.0`)
 SOLVER_NAME=(`awk '/#S/{print $3}' PLOG.0`)
 REFRESH_NUM=(`awk '/#R/{print $2}' PLOG.0`)
REFRESH_NAME=(`awk '/#R/{print $3}' PLOG.0`)

if [[ "x`which parse-trace`" == "x" ]]; then
    echo "ERROR: parse-trace not found!"
    exit 1
fi

# Call parse-trace to generate data files
parse-trace

# For each cycle
for c in $CYCLES; do

    # Create cycle directory
    cycle_dir=`printf "Cycle-%04d" $c`
    mkdir $cycle_dir

    cd $cycle_dir
    for r in $REGIONS; do
        INDICES=`awk '/ '"$r"' /{print $6}' ../PLOG.0 | sort | uniq`
        for i in $INDICES; do
            ln -sf ../$c-$r-$i.data $r-$i.data
            ln -sf ../$c-S$r-$i.data S$r-$i.data
        done
    done

    PLOT="plot-trace-refresh-block.gnu \
          plot-trace-refresh-proc.gnu \
          plot-trace-method-block.gnu \
          plot-trace-method-proc.gnu \
          plot-trace-solver-block.gnu \
          plot-trace-solver-proc.gnu"

    # Generate common gnuplot lines
    for plot in $PLOT; do
        rm -f $plot
        echo "set terminal png size 800,600" >> $plot
        echo "boxwidth = 0.5" >> $plot
        echo "set style fill solid" >> $plot
        echo "set grid" >> $plot
        echo "" >> $plot
        echo "set ylabel \"time\"" >> $plot
        echo "set key below maxrows 2" >> $plot
    done

    # Find cycle time bounds time_min time_max

    time_min=`cat *.data | awk 'BEGIN{mn=1000000}; {if (mn > \$3) { mn = \$3}}; END {print mn; }'`
    time_max=`cat *.data | awk 'BEGIN{mx=-10000000}; {if (mx < \$4) { mx = \$4}}; END {print mx; }'`

    #----------------------------------------
    # Write plot-specific gnuplot lines
    #----------------------------------------

    #----------------------------------------
    plot="plot-trace-refresh-proc.gnu"
    #----------------------------------------
    echo "set output \"trace-refresh-proc.png\"" >> $plot
    echo "set title \"Refresh traces\"" >> $plot
    echo "set xlabel \"proc id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for r in "${REFRESH_NUM[@]}"; do
        n=${REFRESH_NAME[$r]}
        if [[ -e R-$r.data ]]; then
            echo "\"R-$r.data\" u 2:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-method-proc.gnu"
    #----------------------------------------
    echo "set output \"trace-method-proc.png\"" >> $plot
    echo "set title \"Method traces\"" >> $plot
    echo "set xlabel \"proc id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for m in "${METHOD_NUM[@]}"; do
        n=${METHOD_NAME[$m]}
        if [[ -e M-$m.data ]]; then
            echo "\"M-$m.data\" u 2:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-solver-proc.gnu"
    #----------------------------------------
    echo "set output \"trace-solver-proc.png\"" >> $plot
    echo "set title \"Solver traces\"" >> $plot
    echo "set xlabel \"proc id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for s in "${SOLVER_NUM[@]}"; do
        n=${SOLVER_NAME[$s]}
        if [[ -e S-$s.data ]]; then
            echo "\"S-$s.data\" u 2:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot


    #----------------------------------------
    plot="plot-trace-refresh-block.gnu"
    #----------------------------------------
    echo "set output \"trace-refresh-block.png\"" >> $plot
    echo "set title \"Refresh traces\"" >> $plot
    echo "set xlabel \"block id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for r in "${REFRESH_NUM[@]}"; do
        n=${REFRESH_NAME[$r]}
        if [[ -e R-$r.data ]]; then
            echo "\"R-$r.data\" u 1:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-method-block.gnu"
    #----------------------------------------
    echo "set output \"trace-method-block.png\"" >> $plot
    echo "set title \"Method traces\"" >> $plot
    echo "set xlabel \"block id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for m in "${METHOD_NUM[@]}"; do
        n=${METHOD_NAME[$m]}
        if [[ -e M-$m.data ]]; then
            echo "\"M-$m.data\" u 1:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-solver-block.gnu"
    #----------------------------------------
    echo "set output \"trace-solver-block.png\"" >> $plot
    echo "set title \"Solver traces\"" >> $plot
    echo "set xlabel \"block id\"" >> $plot
    k=0
    echo "plot [0:][$time_min:$time_max] \\" >> $plot
    for s in "${SOLVER_NUM[@]}"; do
        n=${SOLVER_NAME[$s]}
        if [[ -e S-$s.data ]]; then
            echo "\"S-$s.data\" u 1:((\$3+\$4)/2.):(boxwidth/2.):((\$4-\$3)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    cd ..
    #

done

