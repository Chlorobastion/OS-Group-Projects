# This is the program used to compile 
CC			= gcc -fopenmp

# This is the name of the executable
PROGRAM		= OutputOpenMP

# This is the name of the c file
SRCS		= Proj4InLine.c

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
	sbatch --time=1 --mem-per-cpu=2G --cpus-per-task=1 --ntasks=1 --nodes=1 --constraint=elves ./run-1.sh
	sbatch --time=1 --mem-per-cpu=1G --cpus-per-task=2 --ntasks=1 --nodes=1 --constraint=elves ./run-2.sh
	sbatch --time=1 --mem-per-cpu=512M --cpus-per-task=4 --ntasks=1 --nodes=1 --constraint=elves ./run-4.sh
	sbatch --time=1 --mem-per-cpu=256M --cpus-per-task=8 --ntasks=1 --nodes=1 --constraint=elves ./run-8.sh
	sbatch --time=1 --mem-per-cpu=128M --cpus-per-task=16 --ntasks=1 --nodes=1 --constraint=elves ./run-16.sh