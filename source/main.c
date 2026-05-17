#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <dirent.h>

// Main GESF3 batch conversion function
void run_tool(int *count, int *success) {

	// Open input directory
	DIR *dir = opendir("sdmc:/3ds/GESF3/input/");

	if (!dir) {

		consoleClear();

		printf("Error: failed opening input directory\n'sdmc:/3ds/GESF3/input/'!\n\nPress START to exit\n");
		return;
	}

	// Look for '.sav' files
	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {

		size_t namelen = strlen(entry->d_name);

		if (namelen > 4 && strcmp(entry->d_name + (namelen - 4), ".sav") == 0 ) {

			// Truncate long file names to keep console output nicely aligned
			if (namelen > 14) {

				printf("Processing '%.11s...'", entry->d_name);
			}

			else {

				printf("Processing '%.14s'", entry->d_name);
			}

			fflush(stdout);

			(*count)++;

			// Open a save file
			char inputpath[512];
			char outputpath[512];

			snprintf(inputpath, sizeof(inputpath), "sdmc:/3ds/GESF3/input/%s", entry->d_name);
			snprintf(outputpath, sizeof(outputpath), "sdmc:/3ds/GESF3/output/%s", entry->d_name);

			FILE *f = fopen(inputpath, "rb");

			if (!f) {

				printf("\n-> Error: failed opening input file!\n");
				continue;
			}

			// Check file length in bytes
			if (fseek(f, 0, SEEK_END) != 0) {

				printf(" -> Error: fseek error!\n");
				fclose(f);
				continue;
			}

			long tmp = ftell(f);

			if (tmp < 0) {

				printf(" -> Error: ftell error!\n");
				fclose(f);
				continue;
			}

			size_t filelen = (size_t)tmp;

			if (filelen > 1024 * 1024) {

				printf("\n-> Error: file too large for a GBA save!\n");
				fclose(f);
				continue;
			}

			if (filelen == 0) {

				printf(" -> Error: empty file!\n");
				fclose(f);
				continue;
			}

			rewind(f);

			// Allocate memory
			uint8_t *data = malloc(filelen);

			if (!data) {

				printf(" -> Error: memory error\n");
				fclose(f);
				continue;
			}

			// Read file into memory and verify size
			size_t read = fread(data, 1, filelen, f);
			fclose(f);

			if (read != filelen) {

				printf(" -> Error: fread error!\n");
				free(data);
				continue;
			}

			// Fix gba EEPROM save files
			for (size_t i = 0; i + 7 < filelen; i += 8) {
				uint8_t tmpbyte;

				for (int j = 0; j < 4; j++) {
					tmpbyte = data[i + j];
					data[i + j] = data[i + 7 - j];
					data[i + 7 - j] = tmpbyte;
				}
			}

			// Open output file
			FILE *out = fopen(outputpath, "wb");

			if (!out) {

				printf("\n-> Error: failed opening output file or folder!\n");
				free(data);
				continue;
			}

			// Write in the output file the fixed data
			size_t written = fwrite(data, 1, filelen, out);

			if (written != filelen) {

				printf("\n-> Error: output writing error!\n");
				fclose(out);
				free(data);
				continue;
			}

			(*success)++;
			printf(" -> Success!\n");

			fclose(out);
			free(data);
		}
	}

	closedir(dir);

	if (!(*count)) {

		consoleClear();

		printf("No '.sav' file found in the input folder\n\nPress START to exit\n");
		return;
	}
}

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	int state = 0;
	int count = 0;
	int success = 0;

	printf("GESF3 - GBA EEPROM Save Fixer 3ds\n\nHello user!\n\n\nPlace Game Boy Advance EEPROM save files in the\n\ninput folder (sdmc:/3ds/GESF3/input/), then press\n\nthe Y button to start converting/fixing them.\n\n\nIf you don't know what 'EEPROM' means or you have\n\nany doubts, check the ReadMe before you proceed.\n\n\nPress Y to start\n\nPress START to exit\n");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();
		u32 kDown = hidKeysDown();

		switch (state) {
			case 0:

				if (kDown & KEY_Y)
					state = 1;
				break;

			case 1:

				consoleClear();
				state = 2;
				break;

			case 2:

				run_tool(&count, &success);

				state = 3;
				break;

			case 3:

				if (count != 0) {

					printf("\n%d save files processed\n", count);
					printf("%d successful operations\n", success);
					printf("%d operations failed\n", count - success);
					printf("\nPress START to exit\n");
				}
				state = 4;
				break;

			case 4:

				break;
		}

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	return 0;
}
