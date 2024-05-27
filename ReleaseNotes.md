
# version 0.1.0

First official version of the BPL library. 

This is not a functional version since there is no way to provide specific data to specific rank/dpu/tasklet, i.e. all tasklets will get the same input data.

On the other hand, this version provides a first attempt to hide host/DPU exchanges and to make it possible to write only one program (vs. two programs, one for the host and one for the DPU)

The "Hello World" project is a good starting point to see the library in action.