EXE ?= SimpleChessEngine

CMAKE := $(shell which cmake 2>/dev/null || echo cmake)

all: $(EXE)

$(EXE):
	@if ! command -v cmake >/dev/null 2>&1; then \
		echo "Error: cmake not found in PATH"; \
		exit 1; \
	fi
	$(CMAKE) -DCMAKE_BUILD_TYPE=Release -S . -B build
	$(CMAKE) --build build
	@if [ -f build/SimpleChessEngine ]; then \
		cp build/SimpleChessEngine $(EXE); \
	elif [ -f build/SimpleChessEngine.exe ]; then \
		cp build/SimpleChessEngine.exe $(EXE).exe; \
	else \
		echo "Error: SimpleChessEngine binary not found in build directory"; \
		exit 1; \
	fi

clean:
	rm -rf build
	rm -f Engine-*

.PHONY: all clean
