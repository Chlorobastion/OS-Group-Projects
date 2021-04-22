# This is the program used to compile 
CC			= gcc

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
	./$(PROGRAM) 32

clean:
	/bin/rm -f $(OBJS) $(PROGRAM)