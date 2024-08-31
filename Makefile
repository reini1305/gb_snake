CC	= $(GBDKDIR)bin/lcc -Wa-l -Wl-m -Wl-j

OBJDIR     = obj
RESDIRS     = res sound
RESDIR     = res
SOUNDDIR = sound
CSOURCES   = $(foreach dir,./,$(notdir $(wildcard $(dir)/*.c))) $(foreach dir,$(RESDIRS),$(notdir $(wildcard $(dir)/*.c)))
ASMS      = $(foreach dir,.,$(notdir $(wildcard $(dir)/*.s))) $(foreach dir,$(RESDIRS),$(notdir $(wildcard $(dir)/*.s)))
OBJS       = $(CSOURCES:%.c=$(OBJDIR)/%.o) $(ASMS:%.s=$(OBJDIR)/%.o)
BINS	   = $(OBJDIR)/snake.gb

# audio-system memory located in  0xdf80 -> 0xdfff
LINKERFLAGS = '-Wl-g .STACK=0xdf80' -Wm-yn"GB_SNEK"

all:	$(BINS)

compile.bat: Makefile
	@echo "REM Automatically generated from Makefile" > compile.bat
	@make -sn | sed y/\\//\\\\/ | grep -v make >> compile.bat

# Compile .c files to .o object files
$(OBJDIR)/%.o:	%.c
	$(CC) $(LCCFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.s
	$(CC) $(LCCFLAGS) -c -o $@ $<

# Compile .c files in "res/" to .o object files
$(OBJDIR)/%.o:	$(RESDIR)/%.c
	$(CC) $(LCCFLAGS) -c -o $@ $<

$(OBJDIR)/%.o:	$(SOUNDDIR)/%.c
	$(CC) $(LCCFLAGS) -c -o $@ $<
$(OBJDIR)/%.o: $(SOUNDDIR)/%.s
	$(CC) $(LCCFLAGS) -c -o $@ $<
	
# Link the compiled object files into a .gb ROM file
$(BINS):	$(OBJS)
	$(CC) $(LINKERFLAGS) -Wm-yC -o $(BINS) $(OBJS)

clean:
	rm -f  $(OBJDIR)/*.*

debug:
	echo $(CSOURCES)
	echo $(ASMS)

