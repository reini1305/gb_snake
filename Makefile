CC	= ../bin/lcc -Wa-l -Wl-m -Wl-j

OBJDIR     = obj
RESDIR     = res
CSOURCES   = $(foreach dir,./,$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIR),$(notdir $(wildcard $(dir)/*.c)))
OBJS       = $(CSOURCES:%.c=$(OBJDIR)/%.o)
BINS	   = $(OBJDIR)/snake.gb

all:	$(BINS)

compile.bat: Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | grep -v make >> compile.bat

# Compile .c files to .o object files
$(OBJDIR)/%.o:	%.c
	$(CC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(CC) $(LCCFLAGS) -c -o $@ $<

# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(CC) $(LCCFLAGS) -Wm-yC -o $(BINS) $(OBJS)

clean:
	rm -f  $(OBJDIR)/*.*

