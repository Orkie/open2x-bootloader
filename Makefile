#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/gp2x_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES		:=	source data
INCLUDES	:=	include
BOOTSTRAP       :=      bootstrap

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	

CFLAGS	:=	-g -Wall -O2 -mtune=arm9tdmi \
		-fomit-frame-pointer \
		-ffast-math \
		-fPIC \
		$(ARCH)
ASFLAGS :=      -fPIC

CFLAGS	+=	$(INCLUDE)

AFLAGS	:=	$(ARCH)
LDFLAGS	=	$(ARCH) -Wl,-Map,$(notdir $@).map -Xlinker --section-start=.crt0=0x3D00000 -Xlinker --defsym=__start_of_heap=0x100000 -Xlinker --defsym=__end_of_heap=0x3D00000

#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS		:=	-lfat -lorcus


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBORCUS)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)


CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BMPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bmp)))
BINFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(BINFILES:.bin=.o) $(BMPFILES:.bmp=.o)\
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@make -C $(BOOTSTRAP) clean
	@rm -fr $(BUILD) *.elf *.bin *.img


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
gp2xboot.img    :       $(OUTPUT).bin bootstrap.bin
	cat $(CURDIR)/../$(BOOTSTRAP)/build/bootstrap.bin $(OUTPUT).bin > ../$@

bootstrap.bin:
		make -C $(CURDIR)/../$(BOOTSTRAP)

$(OUTPUT).bin   :       $(OUTPUT).elf
	$(OBJCOPY) -O binary $< $@

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
%.bin	:	%.bmp
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	bmp2bin -q $< $@

#---------------------------------------------------------------------------------
%.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
