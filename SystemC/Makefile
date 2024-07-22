# The final executable
PROGRAM = traffic

# The source files (*.cc)
SRCS = traffic_ctrl.cc input_gen.cc monitor.cc TB_traffic_ctrl.cc

##################################
## DO NOT CHANGE ANYTHING BELOW ##
##################################

# LIBS = -Wl,-Bstatic -lsystemc -Wl,-Bdynamic -lm -lpthread
LIBS = -lsystemc
LDFLAGS = -g

CC = g++
OPT = -O3
CFLAGS = -Wno-deprecated -Wall -Wextra
EXE = $(PROGRAM)

OBJS = $(SRCS:.cc=.o)

.SUFFIXES: .cc .o .x

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) $(INCDIR) $(LIBDIR) -o $(EXE) $(OBJS) $(LIBS)

TB_traffic_ctrl.o: TB_traffic_ctrl.cc traffic_ctrl.h input_gen.h monitor.h
	$(CC) -c $(CFLAGS) $(INCDIR) TB_traffic_ctrl.cc -o TB_traffic_ctrl.o

traffic_ctrl.o: traffic_ctrl.cc traffic_ctrl.h
	$(CC) -c $(CFLAGS) $(INCDIR) traffic_ctrl.cc -o traffic_ctrl.o

input_gen.o: input_gen.cc input_gen.h
	$(CC) -c $(CFLAGS) $(INCDIR) input_gen.cc -o input_gen.o

monitor.o: monitor.cc monitor.h
	$(CC) -c $(CFLAGS) $(INCDIR) monitor.cc -o monitor.o

clean:
	rm -f $(OBJS) $(EXE) *~
