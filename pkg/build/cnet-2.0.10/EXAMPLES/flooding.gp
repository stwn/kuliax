#
# This file includes commands to plot a simple line-graph using the
# gnuplot plotting software. To display the graph under X-windows,
# execute the command :
# 			gnuplot flooding.gp
#
# By default, the plot will appear in a new window under X.
# Lines in this file beginning with a # are comment lines.
#
#
set title "Efficiency of 3 flooding algorithms"
#
set ylabel "Efficiency (%)"
set xlabel "Seconds since reboot"
#
set grid
#
# Next, we could change the output device from an X window to a postcript
# file if we uncommented the following 2 lines:
#
#set terminal postscript "Times-Roman" 12
#set output "flooding.ps"
#
#
# Finally, we plot the data from the indicated files.
#
# The 3 data files were produced from the output of the command(s):
#
#	cnet -T -W -f 1 -M 1 -m 100 -s FLOODING1
#
#
plot 'flooding1.dat' title 'Flooding-1' with linespoints, \
     'flooding2.dat' title 'Flooding-2' with linespoints, \
     'flooding3.dat' title 'Flooding-3' with linespoints
#
#
# We can specify how long, in seconds, we wish to display this plot.
# By specifying -1, the plot will be displayed until we press return.
#
pause -1
