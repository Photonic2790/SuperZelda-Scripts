Super Zelda Editor and Tiled (www.mapeditor.org) Compatibility Documentation 1.1 : Tiled Map Editor Version 2021.03.23

The 32x32 tile format and limit is hardcoded in LoZ3 ALTTP, editing at the 16x16 level (as in Tiled) eventually must fit back into the original limit and format.

IMO it is best to design in this format externally to your ROM and manage your blockset from files. It gives the freedom to go over the limit, then make cuts where needed
in the same manner that LoZ3 was developed. Basically you can design as complex OW as you want, then do your best to squeeze it into the ROM leaving any cutting room floor 
decisions to the final product. Instead of a limited editor that constantly hinders the workflow.

From Tiled to Zelda3, you will need 160 (all) OW areas as .z16 to rebuild the 32x32 blockset (LoZ3 encoding) and all new 32x32 tilemaps (.z32) using "SuperZed\Maps\SZ16_OW.exe"
This process is automated, SuperZed can extract all the areas as .z16 and once complete SuperZed can batch import all the .z32 files. From Tiled to game testing in 1 minute.

SuperZed has minimal 16x16 OW editing abilities, it is designed around the 32x32 LoZ3 format required to play the game. To edit or view a .z16 in SuperZed hold Conrol when 
opening an OW Area and select the file. SuperZed's 16x16 editing abilities will never match Tiled, things like select & copy/paste are 32x32 based and do not exist in 16 mode.
- I first used Tiled to get ideas for 16 Mode, noticed it's xml formating was easily readable and wrote these basic conversion functions.
- Best part of using an external tool to edit external files is keeping the very important game format 32x32 mode OW editor intact in Super Zelda Editor.

/********************************************************************************************************************************************************************************
Name: Z16_to_TMX.exe
Version: 1.1
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

/********************************************************************************************************************************************************************************
Name: TMX_to_Z16.exe
Version: 1.1
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
in HEX this is what leads into the actual tilemap: 
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