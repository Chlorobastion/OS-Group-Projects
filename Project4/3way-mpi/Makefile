# This is the program used to compile 
CC			= gcc

# This is the name of the executable
PROGRAM		= mpi

# This is the name of the c file
SRCS		= simple_avg_chars.c

# This is any o file that's name matches the file above
OBJS		= $(SRCS:.c=.o)

#######################################################

default: all

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM) $(LDFLAGS)

check:
	./$(PROGRAM)

clean:
	/bin/rm -f $(OBJS) $(PROGRAM)