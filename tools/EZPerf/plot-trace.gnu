set terminal png size 800,600
boxwidth = 0.5
set style fill solid
set grid

set output "trace-refresh.png"
set title "Refresh traces"
set xlabel "block id"
set ylabel "time"
set key below maxrows 2
plot [0:][:] \
     "R-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "R-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "R-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "R-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "R-7.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "R-8.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "R-9.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars

set output "trace-method.png"
set title "Method traces"
set xlabel "block id"
set ylabel "time"

plot [0:][:] \
     "M-0.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "M-1.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "M-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "M-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "M-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "M-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "M-6.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars

set output "trace-refresh-sorted.png"
set title "Sorted refresh traces"
set xlabel "blocks"
set ylabel "time"
plot [0:][:] \
     "SR-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "SR-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "SR-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "SR-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "SR-7.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "SR-8.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "SR-9.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars

set output "trace-method-sorted.png"
set title "Sorted method traces"
set xlabel "blocks"
set ylabel "time"

plot [0:][:] \
     "SM-0.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "SM-1.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "SM-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "SM-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "SM-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "SM-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "SM-6.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars

set output "trace-refresh-endsort.png"
set title "End-sorted refresh traces"
set xlabel "blocks"
set ylabel "time"
plot [0:][:] \
     "RSR-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "RSR-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "RSR-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "RSR-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "RSR-7.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "RSR-8.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "RSR-9.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars

set output "trace-method-endsort.png"
set title "End-sorted method traces"
set xlabel "blocks"
set ylabel "time"

plot [0:][:] \
     "RSM-0.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "order" w boxxyerrorbars, \
     "RSM-1.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "balance" w boxxyerrorbars, \
     "RSM-2.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_deposit" w boxxyerrorbars, \
     "RSM-3.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "gravity" w boxxyerrorbars, \
     "RSM-4.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "ppm" w boxxyerrorbars, \
     "RSM-5.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "pm\\\_update" w boxxyerrorbars, \
     "RSM-6.data" u 0:(($1+$2)/2.):(boxwidth/2.):(($2-$1)/2.) title "expansion" w boxxyerrorbars
