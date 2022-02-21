#include <stdio.h>
#include "tokenizer.h"

void main() {
	for (;;) {
		printf("> ");

		char* line = NULL;
		size_t n = 0;
		getline(&line, &n, stdin);

		Tokenizer tokenizer;
		tok_init(&tokenizer, line);
		for(;;) {
			Token tok = tok_next(&tokenizer);
			switch (tok.type) {
				#define PREAK(...) printf(__VA_ARGS__); break
				case TOKEN_EOF: printf("<eof>"); goto endprint;
				case TOKEN_LIT_STR: PREAK("string<%.*s>", (int)tok.len, tok.start);
				case TOKEN_IDENT: PREAK("ident<%.*s>", (int)tok.len, tok.start);
				case TOKEN_LIT_INT: PREAK("int<%.*s>", (int)tok.len, tok.start);
				case TOKEN_LIT_FLOAT: PREAK("float<%.*s>", (int)tok.len, tok.start);

				case TOKEN_ERR_UNEXPECTED: PREAK("error<unexpected char: %d>", (int)*tok.start);
				case TOKEN_ERR_FLOAT_REQUIRE_EXP: PREAK("error<float: exponent required>");
				case TOKEN_ERR_STRING_REQUIRE_TERMINATION: PREAK("error<string: unterminated>");
				case TOKEN_ERR_COMMENT_REQUIRE_TERMINATION: PREAK("error<comment: unterminated>");
				case TOKEN_ERR_GENERIC: PREAK("error<?>");

				case TOKEN_COMMENT:
				case TOKEN_COMMENT_MULTI:
					PREAK("comment<%.*s>", (int)tok.len, tok.start);

				case TOKEN_STRUCT ... TOKEN_CONTINUE: PREAK("keyword(%.*s)", (int)tok.len, tok.start);
				default: PREAK("%.*s", (int)tok.len, tok.start);
				#undef PREAK
			}
			putchar(' ');
		}

		endprint:
		putchar('\n');
	}

}
