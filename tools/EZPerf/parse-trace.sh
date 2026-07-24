#!/bin/bash

cat PLOG.* >PLOG
CYCLES=`awk '{print $2}' PLOG | sort | uniq`
REGIONS=`awk '{print $4}' PLOG | sort | uniq`
for c in $CYCLES; do
    for r in $REGIONS; do
        INDICES=`awk '/ '"$r"' /{print $5}' PLOG.0 | sort | uniq`
        for i in $INDICES; do
            grep " $r $i " PLOG | awk '{if ($2=='$c') print}' | sort > $c-$r-$i.raw
            awk '/\[/{c=$NF}; /\]/{print c,$NF}' $c-$r-$i.raw >  $c-$r-$i.data
            sort $c-$r-$i.data > $c-S$r-$i.data
            cat $c-$r-$i.data  | \
                awk '{print $2,$1}' | \
                sort | \
                awk '{print $2,$1}' > $c-RS$r-$i.data
            rm $c-$r-$i.raw
        done
    done
    cycle_dir=`printf "Cycle-%04d" $c`
    mkdir $cycle_dir
    cd $cycle_dir
    for r in $REGIONS; do
        INDICES=`awk '/ '"$r"' /{print $5}' ../PLOG.0 | sort | uniq`
        for i in $INDICES; do
            ln -s ../$c-$r-$i.data $r-$i.data
            ln -s ../$c-S$r-$i.data S$r-$i.data
            ln -s ../$c-RS$r-$i.data RS$r-$i.data
        done
    done
    cd ..
done

