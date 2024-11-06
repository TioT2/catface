cf_dir = ../cf

all: out/out.cfexe

out/out.cfexe: out/main.cfobj out/vec.cfobj out/camera.cfobj
	$(cf_dir)/cf_linker -o out/out.cfexe out/main.cfobj out/vec.cfobj out/camera.cfobj
    
out/main.cfobj: src/main.cfasm
	$(cf_dir)/cf_assembler -o out/main.cfobj src/main.cfasm

out/vec.cfobj: src/vec.cfasm
	$(cf_dir)/cf_assembler -o out/vec.cfobj src/vec.cfasm

out/camera.cfobj: src/camera.cfasm
	$(cf_dir)/cf_assembler -o out/camera.cfobj src/camera.cfasm

clean:
	rm -rf out/*

run: out/out.cfexe
	$(cf_dir)/cf_executor out/out.cfexe
