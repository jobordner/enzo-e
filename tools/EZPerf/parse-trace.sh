#!/bin/bash

cat PLOG.* >PLOG
CYCLES=`awk '/\[/{print $2}' PLOG | sort | uniq`
REGIONS=`awk '/\[/{print $4}' PLOG | sort | uniq`
REFRESH_NUM=(`awk '/#R/{print $2}' PLOG.0`)
REFRESH_NAME=(`awk '/#R/{print $3}' PLOG.0`)
METHOD_NUM=(`awk '/#M/{print $2}' PLOG.0`)
METHOD_NAME=(`awk '/#M/{print $3}' PLOG.0`)

parse-trace

for c in $CYCLES; do
##    for r in $REGIONS; do
##        INDICES=`awk '/ '"$r"' /{print $5}' PLOG.0 | sort | uniq`
##        for i in $INDICES; do
##            grep " $r $i " PLOG | awk '{if ($2=='$c') print}' | sort > $c-$r-$i.raw
##            awk '/\[/{c=$NF}; /\]/{print c,$NF}' $c-$r-$i.raw >  $c-$r-$i.data
##            sort $c-$r-$i.data > $c-S$r-$i.data
##            cat $c-$r-$i.data  | \
##                awk '{print $2,$1}' | \
##                sort | \
##                awk '{print $2,$1}' > $c-RS$r-$i.data
##            rm $c-$r-$i.raw
##        done
##    done
    cycle_dir=`printf "Cycle-%04d" $c`
    mkdir $cycle_dir

    cd $cycle_dir
    for r in $REGIONS; do
        INDICES=`awk '/ '"$r"' /{print $5}' ../PLOG.0 | sort | uniq`
        for i in $INDICES; do
            ln -sf ../$c-$r-$i.data $r-$i.data
            ln -sf ../$c-S$r-$i.data S$r-$i.data
            ln -sf ../$c-RS$r-$i.data RS$r-$i.data
        done
    done

    PLOT="plot-trace-refresh.gnu \
          plot-trace-refresh-sorted.gnu \
          plot-trace-method.gnu \
          plot-trace-method-sorted.gnu"

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

    #----------------------------------------
    # Write plot-specific gnuplot lines
    #----------------------------------------

    #----------------------------------------
    plot="plot-trace-refresh.gnu"
    #----------------------------------------
    echo "set output \"trace-refresh.png\"" >> $plot
    echo "set title \"Refresh traces\"" >> $plot
    echo "set xlabel \"block id\"" >> $plot
    k=0
    echo "plot [0:][:] \\" >> $plot
    for r in "${REFRESH_NUM[@]}"; do
        n=${REFRESH_NAME[$r]}
        if [[ -e R-$r.data ]]; then
            echo "\"R-$r.data\" u 0:((\$1+\$2)/2.):(boxwidth/2.):((\$2-\$1)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-method.gnu"
    #----------------------------------------
    echo "set output \"trace-method.png\"" >> $plot
    echo "set title \"Method traces\"" >> $plot
    echo "set xlabel \"block id\"" >> $plot
    k=0
    echo "plot [0:][:] \\" >> $plot
    for m in "${METHOD_NUM[@]}"; do
        n=${METHOD_NAME[$m]}
        if [[ -e M-$m.data ]]; then
            echo "\"M-$m.data\" u 0:((\$1+\$2)/2.):(boxwidth/2.):((\$2-\$1)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot


    #----------------------------------------
    plot="plot-trace-refresh-sorted.gnu"
    #----------------------------------------
    echo "set output \"trace-refresh-sorted.png\"" >> $plot
    echo "set title \"Sorted refresh traces\"" >> $plot
    echo "set xlabel \"sorted blocks\"" >> $plot
    k=0
    echo "plot [0:][:] \\" >> $plot
    for r in "${REFRESH_NUM[@]}"; do
        n=${REFRESH_NAME[$r]}
        if [[ -e SR-$r.data ]]; then
            echo "\"SR-$r.data\" u 0:((\$1+\$2)/2.):(boxwidth/2.):((\$2-\$1)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot

    #----------------------------------------
    plot="plot-trace-method-sorted.gnu"
    #----------------------------------------
    echo "set output \"trace-method-sorted.png\"" >> $plot
    echo "set title \"Sorted method traces\"" >> $plot
    echo "set xlabel \"sorted blocks\"" >> $plot
    k=0
    echo "plot [0:][:] \\" >> $plot
    for m in "${METHOD_NUM[@]}"; do
        n=${METHOD_NAME[$m]}
        if [[ -e SM-$m.data ]]; then
            echo "\"SM-$m.data\" u 0:((\$1+\$2)/2.):(boxwidth/2.):((\$2-\$1)/2.) title \"$n\" w boxxyerrorbars, \\" >> $plot
        fi
    done
    echo "" >> $plot
    sed -i 's/_/\\\\\\_/g' $plot
    gnuplot $plot


    #----------------------------------------
    cd ..
    #

done

