pdf:
	max_print_line=12000000	pdflatex -halt-on-error -output-directory=build master.tex && \
	max_print_line=12000000	pdflatex -halt-on-error -output-directory=build master.tex && \
	cd build && \
	export BIBINPUTS="..:" && \
	bibtex master.aux && \
	cd ..  &&  \
	max_print_line=12000000	pdflatex -halt-on-error -output-directory=build master.tex && \
	max_print_line=12000000	pdflatex -halt-on-error -output-directory=build master.tex

once:
	max_print_line=12000000	pdflatex -halt-on-error -output-directory=build master.tex

clean:
	rm -rf build
	mkdir build

view:
	open build/master.pdf