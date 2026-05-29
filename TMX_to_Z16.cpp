/********************************************************************************************************************************************************************************
TMX_to_Z16.cpp
=================================================================================================================================================================================
        Converts from Tiled TMX "csv, Orthogonal, 32x32 map, 16x16 tilesize, 1 Layer" --- TO --- Super Zelda Editor .Z16 format
        !ONE LAYER! like saving a photoshop .psd to .bmp looses layer data, this converter only reads the first tile map layer.
=================================================================================================================================================================================
        Sample Tiled .tmx file header:
<?xml version="1.0" encoding="UTF-8"?>
<map version="1.5" tiledversion="2021.03.23" orientation="orthogonal" renderorder="right-down" width="32" height="32" tilewidth="16" tileheight="16" infinite="0" nextlayerid="2" nextobjectid="1">
 <layer id="1" name="Tile Layer 1" width="32" height="32">
  <data encoding="csv">
=================================================================================================================================================================================
In HEX this is what leads into the actual tilemap: 
22 63 73 76 22 3E 0D 0A
"decoded text": (including the newline '..' "0x0D 0x0A")
"csv">

=================================================================================================================================================================================
*NOTE*
        It searches for "csv" with quotes, do not name tiled entities csv.
        The XML encoding is irrelevant to this app. As long as it reads "csv" it will begin loading the tilemap exactly 4 bytes after the last quote.
            22 63 73 76 22 3E 0D 0A __ __ __ __ __
                                    ^ tilemap
		It finishes sucessfully if it reads all 1024 tiles, comma between values, 0x0D 0x0A (..) end of line flags row end.
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
		cout << "TMX to Z16 Tilemap Conversion Utility v1.2\n\n    Converts tilemaps from Tiled .tmx UTF-8 to SuperZed .z16 format.\n\n";
		cout << "Proper Usage:\n" << argv[0] << " 'MAP_NAME.TMX' 'MAP_NAME.Z16'\n"; 
		cout << "eg: " << argv[0] <<" Lost_Woods_NW.tmx OW000.z16\n"; 
		return 0;
	}
	
	int digits;
		
	FILE *tiledFile;
	tiledFile = fopen( argv[1], "rb");
	if (tiledFile == NULL) { cout<<"Error loading Tiled file "<<argv[1]<<"\n"; return 0; }
		
    FILE *Z16File = NULL;

	char x16buffer[0x800]; // a straight read right down, 16x16 tilemap 32x32 tiles wide, 2 bytes/tile

	int h = 0;
	int i;
	int j;
	int k;
	int l;

    char text_buf[0x200];

	unsigned char sixteenTile[8];
	for (k = 0; k < 8; k++)
	{
		sixteenTile[k] = 0;
	}

	Z16File = fopen( argv[2], "wb");
	if (Z16File == NULL) { return 0; }

	int foundCSV = 0;

	int hexAddress = 0x0;

	char byte[5];

	short x16tile = 0x8000;
	
	while (foundCSV == 0 && hexAddress <= 0x400)
	{
		// 22 63 73 76
		// "  c  s  v;
		fseek ( tiledFile, hexAddress, SEEK_SET );
		byte[0] = fgetc ( tiledFile );
		if (byte[0] == 0x22) // "
		{
			fseek ( tiledFile, hexAddress + 1, SEEK_SET );
			byte[0] = fgetc ( tiledFile );
			if (byte[0] == 0x63) // c
			{
				fseek ( tiledFile, (hexAddress + 2), SEEK_SET );
				byte[0] = fgetc ( tiledFile );
				if (byte[0] == 0x73) // s
				{
					fseek ( tiledFile, (hexAddress + 3), SEEK_SET );
					byte[0] = fgetc ( tiledFile );
					if (byte[0] == 0x76) // v
					{
						fseek ( tiledFile, (hexAddress + 4), SEEK_SET );
						byte[0] = fgetc ( tiledFile );
						if (byte[0] == 0x22) // "
						{
							hexAddress += 7;
							foundCSV = 1;
						}	
					}				
				}			
			}
		}
		hexAddress += 0x01;
	}

	// cout << "Tile Map Starting HEX Address: " << hexAddress << "\n";
	
/// Read from tmx UTF-8 encoded decimal --- WRITE TO -- x16buffer short (HEX) 1024 tiles, 2048 (0x800) bytes.
	for (h = 0, j = 0; h < 1024; h++, j++)
	{
		// cout << "J: " << j << "  HEXADDRESS: " << hexAddress << "\n"; // debugging output
		digits = 0;
		for (i = 0; i < 5; i++)                    
		{                                     
			fseek ( tiledFile, hexAddress + i, SEEK_SET );
			byte[i] = fgetc ( tiledFile );
		}
		
		// analyzie input for digit count, max tilenum is <4000 so we can't hit 5 digits
		if (byte[0] < 0x30 || byte[0] > 0x39) // if the first char is not a valid decimal value
		{
			cout << "Error-#0 reading tilemap at " << hexAddress << "\n INVALID UTF-8 DECIMAL VALUE: " << byte[0] << "\n";
			return 0;
		}

		for (i = 1; i < 5; i++)
		{
			if ( byte[i] == 0x2C || (h == 1023 && byte[i] == 0x0D) ) // looking for a , or as an exception very last tile a newline mark
			{
				digits = i; // byte[0] is confirmed to be a digit, this utf-8 tilenumber has i digits
				break;
			}
		}
		
		if (digits < 1 || digits > 4) // another safety check, if for some reason we didn't set digits, or it somehow becomes an invalid number
		{
			cout << "Error-#1 reading tilemap at " << hexAddress << "\n INVALID DIGIT COUNT: " << digits << "\n";
			return 0;
		}

		x16tile = 0;

		if (digits == 1)
		{
			x16tile = (byte[0] - 0x30);
		}
		else if (digits == 2)
		{			
			x16tile = (((byte[0] - 0x30) * 10) + (byte[1] - 0x30));
		}
		else if (digits == 3)
		{
			x16tile = (((byte[0] - 0x30) * 100) + ((byte[1] - 0x30)*10) + (byte[2] - 0x30) );
		}
		else if (digits == 4)
		{
			x16tile = (((byte[0] - 0x30) * 1000) + ((byte[1] - 0x30)*100) + ((byte[2] - 0x30)*10) + (byte[3] - 0x30) );			
		}
		x16tile --;

		if (x16tile < 0 || x16tile > 3900)
		{
			cout << "Error-#2 reading tilemap at " << hexAddress << "\n INVALID TILENUMBER: " << x16tile << "\n";
			return 0;
		}

		x16buffer[ h * 2 ] = x16tile & 0xff;
		x16buffer[(h * 2) + 1] = (x16tile>>8) & 0xff;

		if (j == 31) { digits += 2; j = -1; }
		hexAddress += digits + 1;
	}


/// Read from x16buffer shorts (HEX) 1024 tiles, 2048 (0x800) bytes. -- WRITE TO -- Z16 out file (interwoven shorts) same length
	for (i = 0, j = 0, l = 0; i < 0x800; j += 4, l++, i += 8)
	{
		if (l == 16) 
		{ 
			j += 64; 
			l = 0; 
		}

		sixteenTile[1] = x16buffer[j];
		sixteenTile[0] = x16buffer[j + 1];
		sixteenTile[3] = x16buffer[j + 2];
		sixteenTile[2] = x16buffer[j + 3];

		sixteenTile[5] = x16buffer[j + 64];
		sixteenTile[4] = x16buffer[j + 65];
		sixteenTile[7] = x16buffer[j + 66];
		sixteenTile[6] = x16buffer[j + 67];

		fseek( Z16File, i, SEEK_SET );
		fwrite( &sixteenTile, sizeof( sixteenTile ), 1, Z16File );
	}
	
	fseek(Z16File,0,SEEK_END);
	fclose( Z16File );

	cout<<"        Sucessfully converted " << argv[1] << " to " << argv[2] << "\n";

    return 1;
}
