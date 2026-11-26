#include "lexer.h"
#include "imager.h"
#include "navigation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_LEN 1000

int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        printf("Invalid number of arguments.\n"); // Executable name
		return -1; //error
    }

	FAT32 * image = mount_image(argv[1]); // Pointer to FAT32 object with info about the img file

    if(image == NULL) {   // Error case
		free(image);
        return -1;
    }

	printf("Image mounted successfully.\n");

	// Print disk info
	//print_info(image);

	while (1) {
		printf("%s", get_image_name());
		printf("%s/> ", image->cwd_path);

		char *input = get_input();
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		// // Handle exit command
		if (tokens->size == 1 && !strcmp(tokens->items[0], "exit"))
		{
			close_image(image);
			free(input);
			free_tokens(tokens);
			break;
		}

		if (tokens->size == 0) {
			// Empty line: nothing to do
		} else if (!strcmp(tokens->items[0], "exit")) { // incorrect usage of exit
			printf("Usage: exit\n");
		} else if (!strcmp(tokens->items[0], "info")) {
			if (tokens->size == 1)
				print_info(image);
			else
				printf("Usage: info\n");
		} else if (!strcmp(tokens->items[0], "ls")) {
			if (tokens->size == 1)
				cmd_ls(image);
			else
				printf("Usage: ls\n");
		} else if (!strcmp(tokens->items[0], "cd")) {
			if (tokens->size == 2)
				cmd_cd(image, tokens->items[1]);
			else
				printf("Usage: cd [DIRNAME]\n");
		} else {
			printf("%s: command not found\n", tokens->items[0]);
		}

		free(input);
		free_tokens(tokens);
	}
	
	free(image);
	printf("Sucessfully unmounted image.\n");
	printf("Exiting...\n");
	return 0;
}