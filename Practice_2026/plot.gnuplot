set terminal pngcairo size 900,650 enhanced font 'Arial,12'
set output 'ga_plot.png'
set title 'Генетический алгоритм: минимизация 5x^3 - 4 на [1,5]'
set xlabel 'x'
set ylabel 'f(x)'
set grid
set xtics 1,1,5
plot 'func_data.txt' with linespoints lt rgb 'black' pt 7 ps 1.5 lw 2 title 'f(x)', \
     'trace_data.txt' using 1:2 with points pt 7 ps 1.5 lc rgb 'blue' title 'лучшие за поколение', \
     'best_point.txt' using 1:2 with points pt 7 ps 3 lc rgb 'red' title 'окончательный оптимум'
