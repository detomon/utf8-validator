#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "test.h"

static uint8_t* readFile(char const* path, size_t* outSize) {
	size_t size;
	uint8_t* data;
	FILE* file = fopen(path, "rb");

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	data = malloc(size);

	if (!data) {
		return NULL;
	}

	fread(data, sizeof(uint8_t), size, file);

	fclose(file);
	*outSize = size;

	return data;
}

static void handleChunk(uint8_t const* chunk, size_t size) {
	//fwrite(chunk, sizeof(uint8_t), size, stdout);
}

int main() {
	clock_t c;
	size_t dataSize;
	uint8_t* data = readFile("UTF-8-test.txt", &dataSize);

	uint8_t buffer[4096];
	utf8_validator validator = {0};

	c = clock();

	uint8_t const* dataPtr = data;
	size_t inSize = dataSize;
	size_t outSize;

	while (inSize) {
		outSize = sizeof(buffer);
		utf8_validate(&validator, &dataPtr, &inSize, buffer, &outSize);

		if (outSize) {
			handleChunk(buffer, outSize);
		}
	}

	outSize = sizeof(buffer);
	utf8_validate(&validator, NULL, NULL, buffer, &outSize);

	if (outSize) {
		handleChunk(buffer, outSize);
	}

	printf("%lf sec\n", (double) (clock() - c) / CLOCKS_PER_SEC);

	return 0;
}
