# compiler, no flags necessary
CC = gcc

# source files for the server and client
SERVER_SOURCES = myshell.c execute.c parse.c list.c
CLIENT_SOURCES = myshell_cli.c parse.c
DEMO_SOURCES = demo.c

HEADERS = execute.h parse.h list.h # included headers to check for changes

# output executables
SERVER_OUTPUT = myshell
CLIENT_OUTPUT = myshell_cli
DEMO_OUTPUT = demo

# default target
all: $(SERVER_OUTPUT) $(CLIENT_OUTPUT) $(DEMO_OUTPUT)

$(SERVER_OUTPUT): $(SERVER_SOURCES) $(HEADERS)
	$(CC) -o $(SERVER_OUTPUT) $(SERVER_SOURCES) -lpthread

$(CLIENT_OUTPUT): $(CLIENT_SOURCES)
	$(CC) -o $(CLIENT_OUTPUT) $(CLIENT_SOURCES)

$(DEMO_OUTPUT): $(DEMO_SOURCES)
	$(CC) -o $(DEMO_OUTPUT) $(DEMO_SOURCES) -lpthread

# clean up generated files
clean:
	rm -f $(SERVER_OUTPUT) $(CLIENT_OUTPUT) $(DEMO_OUTPUT)
