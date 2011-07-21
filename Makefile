# -*- Mode: makefile -*- 

all: mosaicc
SOURCEDIRS  += src/compiler src/ocamlutil
MLLS        += mlexer.mll
MLYS        += mparser.mly
MODULES     += pretty errormsg lexerhack longarray growArray cabs cabshelper \
	 whitetrack util symbol intern types printcpp printstmt alloy localize view eca mparser mlexer frontm compile main
OBJDIR=build
DEPENDDIR=build/.depend
CAML_NOOPT=1

MOSAICC_MODULES=$(MODULES)
    # Include now the common set of rules for OCAML
include src/ocamlutil/Makefile.ocaml

clean: cleancaml

mosaicc: $(OBJDIR)/mosaicc
$(OBJDIR)/mosaicc : $(MOSAICC_MODULES:%=$(OBJDIR)/%.$(CMO)) \
                        $(MOSAICC_CMODULES:%=$(OBJDIR)/%.$(CMC))
	@$(NARRATIVE) "Linking $(COMPILETOWHAT) $@ $(LINKMSG)"
	$(AT)$(CAMLLINK) -verbose -o $@ \
                    $(MOSAICC_LIBS:%=%.$(CMXA)) \
                    $(MOSAICC_LIBS:%=-cclib -l%) \
                    $^

