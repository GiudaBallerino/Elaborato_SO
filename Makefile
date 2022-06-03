CFLAGS = -Wall -std=gnu99

HEADERDIR = inc
SOURCEDIR = src
OBJDIR = obj

SOURCES := $(shell find $(SOURCEDIR) -name '*.c')
OBJS = $(addprefix $(OBJDIR)/, $(SOURCES:%.c=%.o))

all: $(OBJDIR) client_0 server

client_0: $(OBJS) $(OBJDIR)/client_0.o
	@echo "Making executable: "$@
	@$(CC) $^ -o $@  -lm

server: $(OBJS) $(OBJDIR)/server.o
	@echo "Making executable: "$@
	@$(CC) $^ -o $@  -lm

$(OBJDIR)/client_0.o: client_0.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/server.o: server.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)/$(SOURCEDIR)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -I $(HEADERDIR) -c $< -o $@

clean: wipe
	@rm -vf ${OBJS}
	@rm -rf ${OBJDIR}
	@rm -vf client_0
	@rm -vf server
	@ipcrm -a
	@echo "Removed object files and executables"

wipe:
	@rm -vf ./myDir/*_out
	@rm -vf ./myDir/subfolder/*_out
	@echo "Removed files _out in /myDir"
	@rm -rf /tmp/* || true
	@echo "Removed files in /tmp"

.PHONY: run clean wipe
