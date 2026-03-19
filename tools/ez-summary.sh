#!/bin/bash

# ==============================
# Verify input parameters
# ==============================

if [[ ("x$1" == "x") || ("x$2" != "x") ]]; then
    echo "Usage: $(basename $0) <Enzo-E output file>"
    exit 1
fi

out="$1"

t=${out##*/}

awk 'BEGIN{CONVFMT="%.3f"; b0=-1; bn=-1; c0=-1; cn=-1;nh=-1;nn=-1;np=-1;smp=-1} / time-sim /{te=$2; ts=$NF}; /Simulation cycle/{if (c0==-1) { c0=$NF;}  cn=$NF}; /leaf-blocks/{if (b0==-1) {b0=$NF;} bn=$NF}; /CkNumHosts/{nh=$NF;}; /CkNumNodes/{nn=$NF;}; /CkNumPes/{np=$NF;}; /CONFIG_SMP_MODE/{if ($NF=="Yes") smp="smp 1"; if ($NF=="no") smp="smp 0"; } END{print "time-wall ",te,"time-sim ",ts+0.0,smp,"cycles [",c0,":",cn,"]","blocks [",b0,":",bn,"]","procs [",nh,",",nn,":",np,"]",'\"$t\"';};' $out



