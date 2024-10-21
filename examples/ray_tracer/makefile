all: out/out.cfexe

out/out.cfexe: out/main.cfobj out/vec.cfobj
	cf/cf_linker -o out/out.cfexe out/main.cfobj out/vec.cfobj
    
out/main.cfobj: src/main.cfasm
	cf/cf_assembler -o out/main.cfobj src/main.cfasm

out/vec.cfobj: src/vec.cfasm
	cf/cf_assembler -o out/vec.cfobj src/vec.cfasm

clean:
	rm -rf out/*.cfobj out/*.cfexe
