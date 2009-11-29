In order to build the DFSC audio codec you need the Windows DDK from Microsoft (http://www.microsoft.com/ddk)

The code however assumes you do not have a DDK installed, and uses the msacmdrv.h provided in this directory. Please enable the cpp define USE_DDK_MSACMDRV_H if you would like to build with the header in your DDK and not this one.
