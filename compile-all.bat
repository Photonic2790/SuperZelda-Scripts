windres rsc/z16_to_tmx.rc -O coff -o rsc/z16_to_tmx.res
windres rsc/tmx_to_z16.rc -O coff -o rsc/tmx_to_z16.res
windres rsc/sz16_ow.rc -O coff -o rsc/sz16_ow.res
g++ Z16_to_TMX.cpp -o Z16_to_TMX -lgdi32 rsc/z16_to_tmx.res
g++ TMX_to_Z16.cpp -o TMX_to_Z16 -lgdi32 rsc/tmx_to_z16.res
g++ SZ16_OW.cpp -o SZ16_OW -lgdi32 rsc/sz16_ow.res
pause