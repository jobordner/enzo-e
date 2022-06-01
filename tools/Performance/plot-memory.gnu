set terminal png size 800,600
set termoption noenhanced
set output "plot-memory.png"
set title "Memory usage"
set xlabel "cycle"
set ylabel "memory (MB)"
set key bottom right
set grid
set log y

plot "bytes-curr.data" using ($1):($2/1000000) title "mem-curr" w l lw 2 , \
     "bytes-high.data" using ($1):($2/1000000) title "mem-high" w l lw 2 , \
     "bytes-highest.data" using ($1):($2/1000000) title "mem-highest" w l lw 2
