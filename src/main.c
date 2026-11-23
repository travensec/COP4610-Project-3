#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_LEN 1000

FILE * fp;                  // file pointers for mounting disk image
FILE * fp1;
FILE * fp2;

int main(int argc, char * argv[])
{
    if (argc == 2)
    {
        printf("%s", argv[0]); // Executable name
        printf("%s", argv[1]); // "First" argument
    }

	char *pwd = getenv("PWD");
	while (1) {
		printf(pwd);
		printf("> ");

		char *input = get_input();
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}