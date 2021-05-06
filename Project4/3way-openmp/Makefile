# This is the program used to compile 
CC			= gcc -fopenmp

# This is the name of the executable
PROGRAM		= OutputOpenMP

# This is the name of the c file
SRCS		= Proj4OpenMP.c

# This is any o file that's name matches the file above
OBJS		= $(SRCS:.c=.o)

#######################################################

default: all

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM) $(LDFLAGS)

check:
	./$(PROGRAM) 1

clean:
	/bin/rm -f $(OBJS) $(PROGRAM)

slurm:
	sbatch --time=5 --mem-per-cpu=2G --cpus-per-task=1 --ntasks=1 --nodes=1 --constraint=elves ./run.sh 1
	sbatch --time=5 --mem-per-cpu=2G --cpus-per-task=2 --ntasks=1 --nodes=1 --constraint=elves ./run.sh 2
	sbatch --time=5 --mem-per-cpu=2G --cpus-per-task=4 --ntasks=1 --nodes=1 --constraint=elves ./run.sh 4
	sbatch --time=5 --mem-per-cpu=2G --cpus-per-task=8 --ntasks=1 --nodes=1 --constraint=elves ./run.sh 8
	sbatch --time=5 --mem-per-cpu=2G --cpus-per-task=16 --ntasks=1 --nodes=1 --constraint=elves ./run.sh 16