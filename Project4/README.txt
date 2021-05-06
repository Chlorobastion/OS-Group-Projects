Hello !
In order to run our tests you might wish
to know the commands to run our tests via the MakeFiles.
The commands work in each folder, so don't worry about what
directory you're in!

A bug we had with MPI caused us not to compile, so running the command:
module load foss
seemed to fix it for us, although we still load it with the compilations

:::COMMANDS:::
make OR make all    //compiles the program
make clean          //removes the compiled file
make slurm          //submits all possible tests to slurm, WARNING, makes a lot of output files
make check          //runs the program with the minimal reqs, outputs to terminal
make grade          //runs program once with slurm, outputs to "Output.txt"