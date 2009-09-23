# Streaming Project Makefile
# by Agustin Gianni (agustingianni@gmail.com)
# 

INCLUDES=
OBJS = main.o server.o socket.o stream.o client.o event.o signal.o
CC = gcc
#DEBUG = -ggdb
DEBUG = 
CFLAGS = -Wall $(DEBUG) $(INCLUDES) -Wextra -O2
LFLAGS = -Wall $(DEBUG) -Wextra
MAIN_EXEC = streamer

NAME = Streamer
VERSION = 1

# Directories 
TOPDIR		= $(shell /bin/pwd)

VPATH = ./stream/ ./error/ ./log/ ./net/ ./signal/ ./client/ ./event/ ./benchmark/

all: $(MAIN_EXEC)
	
$(MAIN_EXEC): $(OBJS)
	$(CC) -o $(MAIN_EXEC) $(OBJS) $(CFLAGS) $(LFLAGS)
	
.PHONY : clean
clean :
	@echo "[clean] Cleaning the sources."
	@-rm -f $(OBJS) $(MAIN_EXEC)
	@-rm -f benchmark/benchmark
	@find . -name '*~' -print | xargs rm -f
	
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
distclean: clean
	@echo "[distclean] Clean done."

# Crea un tar.bz2 con los sources y demas cositas
# Le agrega un timestamp basico para ver cuando se creo el tarball
# este timestamp queda en el archivo .TARBALL_CREATION_DATE
# y tambien por las dudas en el nombre del tarball
dist: distclean
	@echo "[dist] Creating distribution Tarball"
	@date > .TARBALL_CREATION_DATE
	@cd .. && tar cjf $(NAME)-v$(VERSION).0.`date +%d%m%Y`.tar.bz2 `basename $(TOPDIR)`/ --exclude=*.mp3
	@sync
	@cd $(TOPDIR)

