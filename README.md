openCL-fun
==========
To run, install the appropriate OpenCL SDK for your platform. Make sure header files are where they're supposed to be and libOpenCL.so is linked
properly. Then:
<code>
./build
</code>
and
<code>
./run
</code>

Now there is a csv of points for each time step called output.csv. You can graph this using octave and the attached
graph function:
<code>
octave
graph
</code>
