#include "lexer.h"
#include "imager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_LEN 1000

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        printf("Invalid number of arguments.\n"); // Executable name
    }

	FAT32 * image = mount_image(argv[1]); // Pointer to FAT32 object with info about the img file

    if(image == NULL) {   // Error case
		free(image);
        return -1;
    }

	printf("Image mounted successfully.\n");

	// Print disk info
	print_info(image);

	while (1) {
		printf("%s", get_image_name());
		printf("> ");

		char *input = get_input();
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		// Handle exit command
		if (tokens->size == 1 && !strcmp(tokens->items[0], "exit"))
		{
			close_image(image);
			free(input);
			free_tokens(tokens);
			break;
		}

		free(input);
		free_tokens(tokens);
	}
	printf("Sucessfully unmounted image.\n");
	printf("Exiting...\n");
	return 0;
}