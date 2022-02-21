all:
	clang src/parser/nodes/*.c src/tokenizer/*.c \
		src/parser/types.c \
		src/parser/symbols.c \
		-g \
		-Iclct clct/*.c \
		-Wall -Wno-unused-function \
		src/test-parser.c \
		-o build/test-parser