/* ************************************************************************** */
/* Author: Hye Yeon Park                                                      */
/* Date: 2/24/2022                                                            */
/* Project: One-Time Pads                                                     */
/* Program: keygen                                                            */
/* Description: Creates a key file of specified length. The characters in the */
/*              file generated will be any of the 27 allowed characters.      */
/* ************************************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main(int argc, char* argv[])
{
	time_t t;
	if (argc < 2)
	{
		fprintf(stderr, "Enter the length of the key\n");
		exit(0);
	}

	int key_length = atoi(argv[1]);

	// Intialize random number generator
	srand((unsigned)time(&t));

	// 27 allowed characters
	const char char_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char* key = malloc(sizeof * key * (key_length + 1));
	for (int i = 0; i < key_length; i++)
	{
		// Randomly choose a character from char_list
		key[i] = char_list[rand() % 27];
	}

	// Add a newline character at the end
	strcat(key, "\n");
	// Print outputs to stdout
	printf("%s", key);
	return 0;
}