EXE = takuzu

all : build

rebuild : clean build

build : 
	@cd src && $(MAKE)
	@cp src/$(EXE) $(EXE)

clean : 
	@cd src && $(MAKE) clean
	@rm -rf $(EXE)
	@cd include && rm -rf *.h.gch

help : 
	@echo "Usage : "
	@echo "  make [all]\t\tCall source Make to build the software"
	@echo "  make clean\t\tRemove all files and outdated software"
	@echo "  make help\t\tDisplay this help"

report : 
	@pdflatex report/report.tex 


.PHONY : all build report clean