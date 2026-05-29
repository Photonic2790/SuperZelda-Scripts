/********************************************************************************************************************************************************************************
Z16_to_TMX.exe
=================================================================================================================================================================================
Converts from Super Zelda Editor .z16 format --- TO --- Tiled 'csv' Format, Orthogonal, 32x32 tile map, 16x16 tile size, 1 Layer .tmx
=================================================================================================================================================================================
From Z16 an interwoven binary file, 1024 tiles, 0x800 bytes --- TO --- TMX xml style decimal UTF-8 encoded Tile Map
=================================================================================================================================================================================
** NOTE **
    Writes SZED16.tsx as the tileset name. This can be changed in a text editor, or name your tileset to match.
        -It is recommended to use the same tileset for all Tiled maps, then change the properties to load the appropriate GFX# and PAL# from disk.
            - This will make all of your maps compatible, and will simulate a GFX/PAL swap in the manner LoZ3 does on a SNES.
            - Tiled is capable of many things not compatible with LoZ3 like multiple tilesets per area and tile flipping for example.

    Creating A Tiled compatible tileset with SuperZed is a 3 step process and requires both SuperZed.exe and image editing software.
        -Step 1: SuperZed\samples\tileset_#.z16   4 files to be loaded into a quad are OW editor 1 at a time (hold CTRL when opening an area to load a .z16 file)
            - Set the appropriate GFX# & PAL#, then take a screenshot (or 2) being sure to capture every tile (ALT & PRINT -> paste to photoshop or gimp)
        -Step 2: Scale the canvas to 512 wide by 2048 tall and align the tiles in the proper order from the screenshot(s)... ->save as png or bmp
        -Step 3: Load the 512x2048 image as a "New tileset" in Tiled, call it "SZED16.tsx" to be compatible with any map made from this application
            -Be sure to keep the tile order correct, double check tile values in Tiled and match them to SuperZed. (test importing/exporting to be safest)
                -tileset0.z16 NW, tileset1.z16 NE, tileset2.z16 SW, tileset3.z16 SE, this puts the tile order to (top) NW / SW / NE / SE (bot) 512x2048
                    - values above 3749 are invalid, they can be ignored or cropped from your tileset image if desired

    Every GFX set and Palette combination you want to use in Tiled needs it's own tileset made like this. I script steps 2 and 3 in Photoshop.
       - I use the naming convention G##P##.png and store images in a folder within my tiled project folder. 
       - LoZ3 ALTTP Light World has 10 different GFX set variations + 4 palette variations for 14 unique tileset images for proper editing.
         - G32P00     : G33P06    : G33P08 : G34P07 : G35P08 Psyc : G36P02 : G37P09 : G39P04 : G41P00,   G41P01,     G41P09 : G43P09 : G47P10 UnderBrdg
         - Pond/Woods : LostWoods : L-Jack : DthMtn : Town/Smithy : Castle : E Ruin : Lake   : LowFalls, Grave/Sanc, Witch  : Desert : Zora/MasterSwd
********************************************************************************************************************************************************************************/

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "rsc\resource.h"

using namespace std;

int main ( int argc, char *argv[] ) 
{
	if ( argc != 3 ) 
	{
		cout << "Z16 to TMX Tilemap Conversion Utility v1.2\n\n    Converts tilemaps from SuperZed .z16 to Tiled .tmx UTF-8 format\n\n";
		cout << "Proper Usage:\n" << argv[0] << " 'MAP_NAME.Z16' 'MAP_NAME.TMX'\n"; 
		cout << "eg: " << argv[0] <<" OW000.z16 Lost_Woods_NW.tmx\n"; 
		return 0;
	}

	int digits;
		
	FILE *tiledFile;
	tiledFile = fopen( argv[2], "wb");
	if (tiledFile == NULL) { cout<<"Error Creating TMX file " << argv[2] << "\n"; return 0; }

	FILE *Z16File = NULL;
	Z16File = fopen( argv[1], "r+b" );
	if (Z16File == NULL) { cout<<"Error Opening Z16 file " << argv[1] << "\n"; return 0; }

	int h = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int l;

	unsigned char byte[2]; // a buffer used to go between little and big endianess short ints
	unsigned char chOut[5]; // character output buffer // max 4 digits per tile, plus 1 "," comma = 5 // and sometimes (2) end of line when needed [0] and [1] 
	unsigned int digitOut[4]; // used in decimal shifting and casting to int to fetch single digits

/**	// X16 format is an e'X'change format, here it exists as a buffer. It's a straight right down read 16x16 blocks 32x32 tiles wide (1 OW area)
	// This is my go between for Z16 (an interwoven internal Zelda 16x16 OW file format) and Tiled's straight read UTF-8 encoding
	// Each 16x16 tile is a short, 1024 make up the map, 2048 (0x800) bytes.*/
	unsigned int x16tile = 0; // the tile number in LoZ3 values	(this is one less than tiled format, 0 there = alpha, 1 in tiled = tile 0 in Zelda)
	
	unsigned char x16Tiles_1[4]; // we read 4 tiles at a time from Z16's interwoven format (these are top left and top right)
	unsigned char x16Tiles_2[4]; // (bottom left bottom right) completing a 32x32 "Blocks set" tile
	for (k = 0; k < 4; k++)
	{
		x16Tiles_1[k] = 0;
		x16Tiles_2[k] = 0;
	}	
	
	// the "X16" buffer, this is the equivalent of an "extensionless" map# file from a ZScream "Too Many 32x32 blocks error" crash output.
	unsigned char x16buffer[0x800];
	
/// This fills the x16buffer in it's format from the z16 file input
// grabs 4 16x16 tiles from the z16 file per cycle, 2 from top row then the 2 beneath them, 16 cycles completes 2 32 tile wide rows.
	for (i = 0, j = 0, l = 0; i < 0x800; j += 4, l++, i += 8)
	{
		if (l == 16) // l counts cycles, at 16 adjust j to draw 2 more rows (j is the top row of 2, so we 'skip' every 2nd row.)
		{ 
			j += 64; // each row is 32 tiles, 64 bytes, (this skips every other row because they are already calculated)
			l = 0; // reset l our loop counter
		}
		// i is from a straight forward read z16 file 4 tiles at a time, i increases 8 bytes per loop
	/// top row buffer is x16Tiles_1 (ONE)
		// top left (assuming first cycle tile 0)
		fseek( Z16File, i, SEEK_SET );
		x16Tiles_1[1] = fgetc ( Z16File );
		fseek( Z16File, i + 1, SEEK_SET );
		x16Tiles_1[0] = fgetc ( Z16File );
		// top right (... tile 1)
		fseek( Z16File, i + 2, SEEK_SET );
		x16Tiles_1[3] = fgetc ( Z16File );
		fseek( Z16File, i + 3, SEEK_SET );
		x16Tiles_1[2] = fgetc ( Z16File );
		
	/// 2nd row buffer is x16Tiles_2 (TWO)
		// bot left (... tile 32)
		fseek( Z16File, i + 4, SEEK_SET );
		x16Tiles_2[1] = fgetc ( Z16File );
		fseek( Z16File, i + 5, SEEK_SET );
		x16Tiles_2[0] = fgetc ( Z16File );
		// bot right (... tile 33)
		fseek( Z16File, i + 6, SEEK_SET );
		x16Tiles_2[3] = fgetc ( Z16File );
		fseek( Z16File, i + 7, SEEK_SET );
		x16Tiles_2[2] = fgetc ( Z16File );
	
		for (k = 0; k < 4; k++) // j is our writing location to x16buffer, j increases 4 bytes per loop
		{
			x16buffer[j + k] = x16Tiles_1[k]; // 4 bytes, 2 tiles, top row
			x16buffer[j + 64 + k] = x16Tiles_2[k]; // same, 2nd row
		}
	}
	// the z16 interwoven format has been converted to a left to right, top down, e"X"change format I call X16
	fseek(Z16File,0,SEEK_END);
	fclose( Z16File );
	
/// Initiating Tile TMX 'csv' format export	
	// .tmx file Header (This is a xml style format written in Hex. They are readable/modifiable in a text editor.)
	char tmxHeader[0x170] =
	{ 
		0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31,
		0x2E, 0x30, 0x22, 0x20, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3D, 0x22, 0x55, 0x54,
		0x46, 0x2D, 0x38, 0x22, 0x3F, 0x3E, 0x0D, 0x0A, 0x3C, 0x6D, 0x61, 0x70, 0x20, 0x76, 0x65, 0x72,
		0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E, 0x35, 0x22, 0x20, 0x74, 0x69, 0x6C, 0x65, 0x64,
		0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x32, 0x30, 0x32, 0x31, 0x2E, 0x30, 0x33,
		0x2E, 0x32, 0x33, 0x22, 0x20, 0x6F, 0x72, 0x69, 0x65, 0x6E, 0x74, 0x61, 0x74, 0x69, 0x6F, 0x6E,
		0x3D, 0x22, 0x6F, 0x72, 0x74, 0x68, 0x6F, 0x67, 0x6F, 0x6E, 0x61, 0x6C, 0x22, 0x20, 0x72, 0x65,
		0x6E, 0x64, 0x65, 0x72, 0x6F, 0x72, 0x64, 0x65, 0x72, 0x3D, 0x22, 0x72, 0x69, 0x67, 0x68, 0x74,
		0x2D, 0x64, 0x6F, 0x77, 0x6E, 0x22, 0x20, 0x77, 0x69, 0x64, 0x74, 0x68, 0x3D, 0x22, 0x33, 0x32,
		0x22, 0x20, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3D, 0x22, 0x33, 0x32, 0x22, 0x20, 0x74, 0x69,
		0x6C, 0x65, 0x77, 0x69, 0x64, 0x74, 0x68, 0x3D, 0x22, 0x31, 0x36, 0x22, 0x20, 0x74, 0x69, 0x6C,
		0x65, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3D, 0x22, 0x31, 0x36, 0x22, 0x20, 0x69, 0x6E, 0x66,
		0x69, 0x6E, 0x69, 0x74, 0x65, 0x3D, 0x22, 0x30, 0x22, 0x20, 0x6E, 0x65, 0x78, 0x74, 0x6C, 0x61,
		0x79, 0x65, 0x72, 0x69, 0x64, 0x3D, 0x22, 0x32, 0x22, 0x20, 0x6E, 0x65, 0x78, 0x74, 0x6F, 0x62,
		0x6A, 0x65, 0x63, 0x74, 0x69, 0x64, 0x3D, 0x22, 0x31, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x3C, 0x74,
		0x69, 0x6C, 0x65, 0x73, 0x65, 0x74, 0x20, 0x66, 0x69, 0x72, 0x73, 0x74, 0x67, 0x69, 0x64, 0x3D,
		0x22, 0x31, 0x22, 0x20, 0x73, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x3D, 0x22, 0x53, 0x5A, 0x45, 0x44,
		0x31, 0x36, 0x2E, 0x74, 0x73, 0x78, 0x22, 0x2F, 0x3E, 0x0D, 0x0A, 0x20, 0x3C, 0x6C, 0x61, 0x79,
		0x65, 0x72, 0x20, 0x69, 0x64, 0x3D, 0x22, 0x31, 0x22, 0x20, 0x6E, 0x61, 0x6D, 0x65, 0x3D, 0x22,
		0x54, 0x69, 0x6C, 0x65, 0x20, 0x4C, 0x61, 0x79, 0x65, 0x72, 0x20, 0x31, 0x22, 0x20, 0x77, 0x69,
		0x64, 0x74, 0x68, 0x3D, 0x22, 0x33, 0x32, 0x22, 0x20, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x3D,
		0x22, 0x33, 0x32, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x64, 0x61, 0x74, 0x61, 0x20, 0x65,
		0x6E, 0x63, 0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3D, 0x22, 0x63, 0x73, 0x76, 0x22, 0x3E, 0x0D, 0x0A
	};	

	fwrite ( ( char* ) &tmxHeader, sizeof(tmxHeader), 1, tiledFile );	

	int hexAddress = 0x170; // end of Hardcoded header. I recommend the needed .tsx tileset naming convention G##P##.tsx Gfx number and Palette, it ends neatly at 0x170.
				// note, the importer reads the header to find the start of the tilemap looking for "csv" and adding a few bytes. not a hardcoded location like this writes
	
/// This reads each 16x16 tile from the X16 buffer (created above) and writes the proper decimal output to UTF-8 encoding for Tiled format. ##,##,##RowEnd,..##,continues
	for (h = 0, j = 1; h < 1024; h++, j++)
	{
		// cout << "J: " << j << "  HEXADDRESS: " << hexAddress << "\n"; // debugging output to terminal
		digits = 0; // reset it for error checking
		for (i = 0; i < 2; i++)                    
		{                                     
			byte[i] = x16buffer[(h * 2) + i];
		}
		x16tile = (byte[1] << 8) + byte[0] + 1; // this is the Tiled format (+ 1) tile number

		// analyzie input for digit count, max tilenum is <4000 so we can't hit 5 digits
		if (x16tile < 0 || x16tile > 4000)
		{
			cout << "Error-#0 reading tilemap at " << h << "\n"; // this can get triggered if there are endian issues in the input, or invalid tile values
			return 0;
		}

		if(x16tile < 10) // pretty simple way to count digits (of a low number)
		{
			digits = 1;
		}
		else if(x16tile < 100) 
		{
			digits = 2;
		}
		else if(x16tile < 1000) 
		{
			digits = 3;
		}
		else if(x16tile < 4000) 
		{
			digits = 4;
		}

		
		if (digits < 1 || digits > 4) // another safety check, if for some reason we didn't set digits, or it' an invalid number
		{
			cout << "Error-#1 reading tilemap at " << h << "\n";
			return 0;
		}

		if (digits == 1) // a slightly different algorithm depending on the number of digits (decimal) in the current tile (x16tile)
		{
			// digitOut[0] = x16tile;
			chOut[0] = x16tile + 0x30; // add 0x30 to the value to get the proper UTF-8 character
			chOut[1] = 0x2C; // "," comma value seperater

		/// write 1 digit and 1 comma
			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[0], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[1], 1, 1, tiledFile );
			hexAddress++;
		}
		else if (digits == 2)
		{
			digitOut[0] = x16tile / 10; // first calculate the value to a variable, for use in the next digits, int rounds down. this is the "tens" value
			chOut[0] = digitOut[0] + 0x30; // then add 0x30 to the value to get the proper UTF-8 character
			digitOut[1] = x16tile - (digitOut[0] * 10); // the next digit is less the first * it's place, this continues in the 3 and 4 digit algorithm with 100s and 1000s
			chOut[1] = digitOut[1] + 0x30;
			chOut[2] = 0x2C;

		/// write 2 digits and 1 comma
			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[0], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[1], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[2], 1, 1, tiledFile );
			hexAddress++;
		}
		else if (digits == 3)
		{
			digitOut[0] = x16tile / 100; // hundreds
			chOut[0] = digitOut[0] + 0x30;
			digitOut[1] = (x16tile - (digitOut[0] * 100)) / 10; //  ( tile number [minus] (amount of hundreds [times] 100) ) divided by 10 = amount of tens
			chOut[1] = digitOut[1] + 0x30;
			digitOut[2] = x16tile - (digitOut[0] * 100) - (digitOut[1] * 10);
			chOut[2] = digitOut[2] + 0x30;
			chOut[3] = 0x2C;

		/// write 3 digits and 1 comma
			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[0], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[1], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[2], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[3], 1, 1, tiledFile );
			hexAddress++;
		}
		else if (digits == 4)
		{
			digitOut[0] = x16tile / 1000; // thousands
			chOut[0] = digitOut[0] + 0x30;
			digitOut[1] = (x16tile - (digitOut[0] * 1000)) / 100; // hundreds
			chOut[1] = digitOut[1] + 0x30;
			digitOut[2] = (x16tile - (digitOut[0] * 1000) - (digitOut[1] * 100)) / 10; // tens
			chOut[2] = digitOut[2] + 0x30;
		///  ( tile number [minus] (amount of thousands [times] 1000) [minus] (amount of hundreds [times] 100) ) [minus] (amount of tens [times] 10) = amount of ones
			digitOut[3] = x16tile - (digitOut[0] * 1000) - (digitOut[1] * 100) - (digitOut[2] * 10); // ones
			chOut[3] = digitOut[3] + 0x30;
			chOut[4] = 0x2C;	

		/// write 4 digits and 1 comma
			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[0], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[1], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[2], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[3], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[4], 1, 1, tiledFile );
			hexAddress++;
		}

		if (h == 1023) hexAddress --; // the very last comma needs to be clipped before writing the "tail" to the Tiled file

		if (j == 32) // j is the loop counter checking for row end to insert the two byte 0x0D 0x0A, the Tiled format row end flag
		{ 
			chOut[0] = 0x0D;
			chOut[1] = 0x0A;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[0], 1, 1, tiledFile );
			hexAddress++;

			fseek ( tiledFile, hexAddress, SEEK_SET );
			fwrite ( ( char* ) &chOut[1], 1, 1, tiledFile );
			hexAddress++;
			j = 0; // reset j
		}
	}
/// finished writing the tilemap to UTF-8 encoding,

/// Write the closing "Tail" to the file
	char tmxTail[0x1C] =
	{ 
		0x3C, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x3E, 0x0D, 0x0A, 0x20, 0x3C, 0x2F, 0x6C, 0x61, 0x79, 0x65,
		0x72, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x6D, 0x61, 0x70, 0x3E, 0x0D, 0x0A
	};	

	fseek ( tiledFile, hexAddress, SEEK_SET );
	fwrite ( ( char* ) &tmxTail, sizeof(tmxTail), 1, tiledFile );

/// Close the last file.
	fclose ( tiledFile );

	cout<<"        Successfully converted " << argv[1] << " to " << argv[2] << "\n";

	return 1; // Good Completion
}
