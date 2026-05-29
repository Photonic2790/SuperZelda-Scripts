/*************************************\
:   SuperZed Z16 Overworld Compiler   :
:             SZ16_OW.cpp             :
\*************************************/

/**********************************************************************************\
: This program is designed to read all of the 160 .z16 files that make up the full :
: Overworld for TLoZ3-alttp and convert them into .z32 files while building a new  :
: blockset of all the 'unique' 32x32 tiles that exist in the 16x16 tile maps.      :
:                                                                                  :
: If successful it will compress the blockset to SNES 48bit format and             :
: import the new tiles into the rom file given as the command line argument.       :
:                                                                                  :
: The user then needs to run: 'Edit->Import All OW Z32 Files' in SuperZed.         :
: Once all .z32s have been imported the rom is ready for playing or further edits. :
\**********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "rsc\resource.h"

int main ( int argc, char *argv[] )
{
	if (argc != 2) 
	{
		printf("SuperZed External 16x16 Overworld Compiler v1.2\n\n    Usage: SZ16_OW.exe ROM.FILE\n\n    Requires all 160 'OW###.z16' files in the directory.\n\n    Writes 160 new OW###.z32 files and imports the new 32x32 blockset to the ROM\n\n    Run 'Edit->Import All Z32 Files' in SuperZed to import the new .z32 files\n");
	}
    //  else
    {

		FILE *Z16_File = NULL; // read only .z16 (all 160 OW###.z16 expected in the directory.  ### = 00 - 159 in decimal)

        FILE *Z32_File = NULL; // write .z32 (will write all 160 OW###.z32 files, unless blockset limit is reached)  

        FILE *AllZ32s_File = NULL; // a compilation of every Z32 file in order 0 - 159  

        FILE *log_File = fopen("SZ16_OW-log.txt", "wb" ); // a log file, text outputs to terminal and this file

        char text_buf[50]; // for automated filenames and text output

        FILE *rom_File = fopen( argv[1], "r+b" );
		if (rom_File == NULL) 
		{
			sprintf(text_buf,"\nFailed opening rom file '%s'\n", argv[1]);
			printf(text_buf);
			fwrite( &text_buf, sizeof( text_buf ), 1, log_File );
			// return 0; // letting this slide for now so one can calculate a blockset count without touching a rom file
		}

		int areaNum; // literally the SuperZed area number, 00 - 3F Light World, 40 - 7F dark world, 80 - 9F extras (Zora falls, triforce room, mastersword, hyrule bridge...)
		int i; // a counter
		int j; // current location in current .z32 file output
		int k; // another counter, sometimes location in current .z16 input file, sometimes in calculating blockSet output buffer location
		int l; // holds location of next new 32x32 tile to write into blockSet output

		unsigned short blocksCount = 0; // a count of the total number of currently found unique 32x32 tiles in all of the .z16 files read
		//  later, a count as we step through 4 at a time to convert from our series of 64bit 32x32 tiles to snes format of 48bit interwoven 32x blocksets

		unsigned char byte = 0x00; // char to char input to rom file

		unsigned char blockSet64[0x11538]; // a buffer to hold our 32x32 tileset (64-bit values, 8 bytes per block) max 8872
		for (i = 0; i < 0x11538; i++)
		{
			blockSet64[i] = 0;
		}

		unsigned char sixteenTile[8]; // 8 bytes that are 4 shorts (16-bit 16x16 tilenumbers)--- [Top Left] [Top Right] [Bot Left] [Bot Right]
		for (i = 0; i < 8; i++)
		{
			sixteenTile[i] = 0;
		}

		unsigned short thirtytwoTile = 0x8000; // this is calculated using the blocksCount var
        
		sprintf(text_buf,"Initiating Z16 to Z32 conversion.");
		printf(text_buf);
		fwrite( &text_buf, sizeof( text_buf ), 1, log_File );

/// START READING Z16S AND WRITING Z32S - NEW 64BIT BLOCKSET COMPILING

		AllZ32s_File = fopen( "zelda3_overworld.dat", "wb" );
		for (areaNum = 0; areaNum <= 0x9F; areaNum++) // it's an all or nothing operation, 160 .z16 files in and 160 .z32 files out + blockset
		{
			// read our current input .z16 file into Z16_File buffer
			sprintf(text_buf,"OW%03d.z16",areaNum);
			Z16_File = fopen( text_buf, "r+b" );
			if (Z16_File == NULL) 
			{ 
				sprintf(text_buf,"\nError opening 'OW%03d.z16'     \n",areaNum);
				printf(text_buf);
				fwrite( &text_buf, sizeof( text_buf ), 1, log_File );
				fseek(rom_File,0,SEEK_END);
				fclose( rom_File );
				fclose( log_File );
				return 0; 
			}

			// open up our current output .z32 file, Z32_File buffer
			sprintf(text_buf,"OW%03d.z32",areaNum);
			Z32_File = fopen( text_buf, "wb" );
			if (Z32_File == NULL)
			{ 
				sprintf(text_buf,"\nError opening 'OW%03d.z16'     \n",areaNum);
				printf(text_buf);
				fwrite( &text_buf, sizeof( text_buf ), 1, log_File );
				fseek(rom_File,0,SEEK_END);
				fclose( rom_File );
				fclose( log_File );
				fseek(Z16_File,0,SEEK_END);
				fclose( Z16_File );
				return 0; 
			}
	
			for (i = 0, j = 0; i < 0x800; j += 2) // i increments in another for loop within this one, as if i+=8 here
			{
				// uint64_t sixteenTile = byte0<<56 | byte1<<48 | byte2 << 40 | byte3 << 32 | byte4 << 24 | byte5 << 16 | byte6 << 8 | byte7;
				for (k = 0; k < 8; k++, i++) // incrementing i here within the loop that uses it as a cutoff value, (i+=8) after this
				{
					fseek( Z16_File, i, SEEK_SET );
					sixteenTile[k] = fgetc ( Z16_File );
				}
		
				l = blocksCount * 8; // l is now the total number of newly created 32x32 tiles in the tileset * 8 (8 byte format reading)

				if (l >= 0x11539) // this is very tight and may or may not clip the last tile while going into blockSet48, to be investigated
				{
					sprintf(text_buf,"\nERROR creating blockSet64 - TOO MANY UNIQUE TILES \n\nDuring conversion of OW%03d\n",areaNum);
					printf(text_buf);
					fwrite( &text_buf, sizeof( text_buf ), 1, log_File );
					fseek(rom_File,0,SEEK_END);
					fclose( rom_File );
					fclose( log_File );
					fseek(Z16_File,0,SEEK_END);
					fclose( Z16_File );			
					fseek(Z32_File,0,SEEK_END);
					fclose( Z32_File );
					fseek(AllZ32s_File,0,SEEK_END);
					fclose( AllZ32s_File );
					return 0; 
				}

				for (k = 0; k <= l; k += 8) // this loop grows as the tileset grows, here it checks the entire blockset for a match
				{
					if ( (sixteenTile[0] == blockSet64[k+0]) &&
						 (sixteenTile[1] == blockSet64[k+1]) &&
						 (sixteenTile[2] == blockSet64[k+2]) &&
						 (sixteenTile[3] == blockSet64[k+3]) &&
						 (sixteenTile[4] == blockSet64[k+4]) &&
						 (sixteenTile[5] == blockSet64[k+5]) &&
						 (sixteenTile[6] == blockSet64[k+6]) &&
						 (sixteenTile[7] == blockSet64[k+7]) )
					{
						thirtytwoTile = k / 8; // the current 32x32 tile has already been created in our blockset, k will hold that blocks number * 8 bytes for this format
						goto matched; // kick it on over to matched where we add this 32x32 tile to our current .z32 output file
					}
				}
				// no match found in blockSet
				thirtytwoTile = blocksCount; // starts at 0, both of these are uint16_t -or- unsigned shorts -or- 2 bytes
				for (k = 0; k < 8; k++) // byte by byte write the new 32x32 block 64-bit value into the 32x32 tileset, blockset.bks
				{
					blockSet64[l + k] = sixteenTile[k];
				}
				blocksCount++; // we only get here if we didn't find a match remember
				
			matched:; // now we have a matching 32x32 tile in our blockset weither it was there before this cycle or not
	
				fseek( Z32_File, j, SEEK_SET ); // explicitly setting the location in the .z32 output file, (probably unnecessarily as I think about it)
				fwrite( &thirtytwoTile, sizeof( thirtytwoTile ), 1, Z32_File ); // write this loops 32x32 tile number (aka it's match in the new blockset buffer)
				fwrite( &thirtytwoTile, sizeof( thirtytwoTile ), 1, AllZ32s_File ); 
			}

			fseek(Z16_File,0,SEEK_END);
			fclose( Z16_File );	
			fseek(Z32_File,0,SEEK_END);
			fclose( Z32_File );			
			sprintf(text_buf,"\nCompleted Area %03d : Current Block Count = %d     ",areaNum, blocksCount);
			printf(text_buf);
			fwrite( &text_buf, sizeof( text_buf ), 1, log_File );	
		}

		fseek(AllZ32s_File,0,SEEK_END);
		fclose( AllZ32s_File );

/// END READING Z16S AND WRITING Z32S - NEW 64BIT BLOCKSET BUILT
		sprintf(text_buf,"\nFinished Z32 and BlockSet64 creation.              ");
		printf(text_buf);
		fwrite( &text_buf, sizeof( text_buf ), 1, log_File );	

/// START COMPRESSING BLOCKSET TO 48BIT SNES FORMAT
		unsigned char blockSet48[4][0x3402]; // To insert to rom file
		for (i = 0; i < 0x3402; i++)
		{
			for (j = 0; j < 4; j++)
			{
				blockSet48[j][i] = 0;
			}
		}

/// sixPack refers to SNES LoZ3 48bit format [tile0 Top Left] [tile1 TL] [tile2 TL] [tile3 TL] [(t0TL << 4)t1TL] [(t2TL << 4)t3TL] 6 bytes in each of 4 arrays per 4 32x32 tile
///                  // ^ continues in this format Top Right, Bottom Left, Bottom Right. Completing a set of 4 32x32 tiles of 4 16x16 tiles each. (24 bytes total)
		unsigned char sixPack[4][6]; // sixPack[0] = top left, [1] tr, [2] bl, [3] br (of 4 32x32 tiles)
		for (i = 0; i < 6; i++)
		{
			for (j = 0; j < 4; j++)
			{
				sixPack[j][i] = 0;
			}
		}

/// eightPack is the 64bit 32x32 tile value found in blockSet64 from first loop reading all z16s, loading four blocks at a time to prepare for conversion to 48bit snes sixPack
		unsigned char eightPack[4][8]; // eightPack[0] = top left, [1] tr, [2] bl, [3] br (of 4 32x32 tiles)
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 4; j++)
			{
				eightPack[j][i] = 0;
			}
		}

		blocksCount = 0; // reset this var for the coming loop

/// The blockset64 to blockset48 conversion loop, do this 0x8AA times, totaling 8872 32x32 tiles (MAX available to LoZ3)
		for (i = 0; i < 0x33FC; i += 6)
		{
			l = blocksCount * 8; // blocksCount increases by 4 per loop, this grabs the next starting location to load 4 tiles from blockSet64 buffer
			for (k = 0; k < 8; k++, l++)
			{
				for (j = 0; j < 4; j++) // tile[j]
				{
					eightPack[j][k] = blockSet64[l + (j * 8)];
				}
			}

			for (k = 0; k < 4; k++)
			{
				for (j = 0; j < 4; j++)
				{
					sixPack[k][j] = eightPack[j][(k * 2) + 1];
				}
				sixPack[k][4] = (eightPack[0][k * 2]<<4) | (eightPack[1][k * 2]&0xF);
				sixPack[k][5] = (eightPack[2][k * 2]<<4) | (eightPack[3][k * 2]&0xF);
			}

			l = (blocksCount / 4) * 6; // calculating next write location into blockSet48[#] out buffers
			for (k = 0; k < 6; k++, l++)
			{
				for (j = 0; j < 4; j++)
				{
					blockSet48[j][l] = sixPack[j][k];
				}
			}
			
			blocksCount += 4;
		}

		sprintf(text_buf,"\nFinished BlockSet48 creation, inserting to rom.       ");
		printf(text_buf);
		fwrite( &text_buf, sizeof( text_buf ), 1, log_File );	

		if (rom_File == NULL) // by doing this here I let as much of the program run as possible without a rom file present, to calculate blocks count for example
		{
			sprintf(text_buf,"\nFailed opening rom file '%s'                  \n", argv[1]);
			printf(text_buf);
			fwrite( &text_buf, sizeof( text_buf ), 1, log_File );
			fseek(rom_File,0,SEEK_END);
			fclose( rom_File );
			fclose( log_File );
			return 0; 
		}

		for (i = 0; i < 0x3400; i++)
		{
			byte = blockSet48[0][i];
			fseek( rom_File, 0x18000 + i, SEEK_SET );
			fwrite( &byte, sizeof( byte ), 1, rom_File );

			byte = blockSet48[1][i];
			fseek( rom_File, 0x1B400 + i, SEEK_SET );
			fwrite( &byte, sizeof( byte ), 1, rom_File );

			byte = blockSet48[2][i];
			fseek( rom_File, 0x20000 + i, SEEK_SET );
			fwrite( &byte, sizeof( byte ), 1, rom_File );

			byte = blockSet48[3][i];
			fseek( rom_File, 0x23400 + i, SEEK_SET );
			fwrite( &byte, sizeof( byte ), 1, rom_File );
        }

		fseek(rom_File,0,SEEK_END);
		fclose( rom_File );

		sprintf(text_buf,"\n\nSZ16_OW.exe has successfully completed!                         ");
		printf(text_buf);
		fwrite( &text_buf, sizeof( text_buf ), 1, log_File );	

		sprintf(text_buf,"\n\nRun 'Edit->Import All Z32 Files' in SuperZed.exe to finish the process.\n");
		printf(text_buf);
		fwrite( &text_buf, sizeof( text_buf ), 1, log_File );

		fclose( log_File );
    }

    return 1;
}
