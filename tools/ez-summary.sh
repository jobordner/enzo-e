#!/bin/bash

# ==============================
# Verify input parameters
# ==============================


while true; do
    out="$1"
    if [[ "x$out" == "x" ]]; then break; fi

    t=${out##*/}
    awk 'BEGIN{ \
            CONVFMT="%.3f"; bh=0; b0=-1; bn=-1; c0=-1; cn=-1;nh=-1;nn=-1;np=-1;smp=-1; rnd=0; inp=0} \
    /Input File name /{inp=$NF}; \
    / time-sim / {te=$2; ts=$NF}; \
    / Simulation redshift /{tr=$NF}; \
    /Parameters  Mesh:root_rank/{mr=$NF}; \
    /Parameters  Mesh:root_blocks/{mb=$(NF-1)}; \
    /Parameters  Mesh:root_size/{ms=$(NF-1)}; \
    /Parameters  Adapt:max_level/{ml=$NF}; \
    /Simulation cycle/ {if (c0==-1) { c0=$NF;}  cn=$NF}; \
    /leaf-blocks/{if (b0==-1) {b0=$NF;} bn=$NF}; \
    /bytes-highest/{if (bh < $NF) bh=$NF; } \
    /CkNumHosts/{nh=$NF;}; \
    /CkNumNodes/{nn=$NF;}; \
    /CkNumPes/{np=$NF;}; \
    /CONFIG_SMP_MODE/{if ($NF=="Yes") smp="1"; if ($NF=="no") smp="0"; } \
    /Using randomized message/{rnd=1}; \
     END{ftr = sprintf ("%.2f",tr); \
         fts = sprintf ("%.2f",ts); \
         fte = sprintf ("%.2f",te); \
         fbh = sprintf ("%.2f",bh/1024./1024./1024.); \
         fc0 = sprintf ("%d",c0); \
         fcn = sprintf ("%d",cn); \
   print "( mesh",mr,ms,mb,ml,\
       ") ( charm",rnd,smp,\
       ") ( procs",nh,nn,np,\
       ") ( time",fte,fts,ftr,\
       ") ( cycles",fc0,fcn,\
       ") ( blocks",b0,bn,\
       ") ( gbytes",fbh,\
       ")",inp};' $out
    shift
done



