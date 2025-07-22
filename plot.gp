set title 'Macrocell BS Power Breakdown by Component'
set xlabel 'RF Output Power (% of Max)'
set ylabel 'Power Consumption (W)'
set grid
set key outside top center horizontal
set xrange [0:100]
set yrange [0:700]
set style fill transparent solid 0.7

# Component definitions
PA(x) = column(2)
RF(x) = column(3)
BB(x) = column(4)
DC(x) = column(5)
CO(x) = column(6)
PS(x) = column(7)
Sleep(x) = column(8)

plot 'components.dat' using 1:2 with filledcurves title 'PA' lc rgb '#FF0000', \
     '' using 1:(PA+RF) with filledcurves title 'RF' lc rgb '#00FF00', \
     '' using 1:(PA+RF+BB) with filledcurves title 'BB' lc rgb '#0000FF', \
     '' using 1:(PA+RF+BB+DC) with filledcurves title 'DC' lc rgb '#FF00FF', \
     '' using 1:(PA+RF+BB+DC+CO) with filledcurves title 'CO' lc rgb '#00FFFF', \
     '' using 1:(PA+RF+BB+DC+CO+PS) with filledcurves title 'PS' lc rgb '#FFFF00', \
     '' using 1:(Sleep) with lines title 'Sleep' lw 2 lc rgb '#000000', \
     '' using 1:9 with lines title 'Total' lw 2 lc rgb '#000000'
