# Define the output terminal and file
set terminal png
set output 'Cl.png'

set xrange [1:1000];
set yrange [0:0.02];

# Set linestyle 1 to blue (#0060ad)
set style line 1 \
    linecolor rgb '#0060ad' \
    linetype 1 linewidth 1 \
    pointtype 7 pointsize 0.5

plot 'postProcessing/forceCoeffs1/0/coefficient.dat' using 0:5 with linespoints linestyle 1 title 'Cl'

# Reset the terminal
reset
