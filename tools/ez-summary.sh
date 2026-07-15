#!/bin/bash

# ==============================
# Verify input parameters
# ==============================


while true; do
    out="$1"
    if [[ "x$out" == "x" ]]; then break; fi

    t=${out##*/}
    awk 'BEGIN{ \
            CONVFMT="%.3f"; bh=0; b0=-1; bn=-1; c0=-1; cn=-1;nh=-1;nn=-1;np=-1;smp=-1; rnd=0; inp=0; st=" XX ";} \
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
    /BEGIN ENZO/{st=" -X "; }; \
    /END ENZO/{st=" -- "; }; \
    /CONFIG_SMP_MODE/{if ($NF=="Yes") smp="1"; if ($NF=="no") smp="0"; } \
    /Using randomized message/{rnd=1}; \
     END{ \
         t_h=int(te/3600); \
         t_m=int((te%3600)/60); \
         t_s=int(te%60); \
         fhms = sprintf ("%02d:%02d:%02d",t_h,t_m,t_s); \
         f_time = sprintf ("%s %.1f %.2f",fhms,ts,tr); \
         f_mem = sprintf ("%5.1f GB",bh/1024./1024./1024.); \
         f_cycle = sprintf ("%d",cn); \
         f_block = sprintf ("%d",bn); \
         f_mesh = sprintf ("N%d:%d-L%d",ms,mb,ml); \
         f_charm = sprintf ("rs%d%d",rnd,smp); \
         f_proc = sprintf ("%d:%d/%d",nh,nn,np); \
   print st,mr,\
       f_mesh,\
       f_charm,\
       f_proc,\
       f_time,\
       f_cycle,\
       f_block,\
       f_mem,\
       inp};' $out
    shift
done



