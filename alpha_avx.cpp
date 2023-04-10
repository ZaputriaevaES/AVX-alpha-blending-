
// g+++ 7-SSEimgAlpha.cpp -O3 -msse4.2

//90-100 fps

#include <stdint.h>
#include <string.h>

#include "C:\Users\zaput\Documents\TXLib\TX\TXLib.h"
#include <immintrin.h>
#include <x86intrin.h>
#include <smmintrin.h>

//-------------------------------------------------------------------------------------------------

typedef RGBQUAD (&scr_t) [600][800];

inline scr_t LoadImage (const char* filename)
    {
    RGBQUAD* mem = NULL;
    HDC dc = txCreateDIBSection (800, 600, &mem);
    txBitBlt (dc, 0, 0, 0, 0, dc, 0, 0, BLACKNESS);

    HDC image = txLoadImage (filename);
    txBitBlt (dc, (txGetExtentX (dc) - txGetExtentX (image)) / 2,
                  (txGetExtentY (dc) - txGetExtentY (image)) / 2, 0, 0, image);
    txDeleteDC (image);

    return (scr_t) *mem;
    }

//-------------------------------------------------------------------------------------------------

const char I = 255u,
           Z = 0x80u;

const __m256i _255 =  _mm256_set_epi8 (0,I,0,I, 0,I,0,I, 0,I,0,I, 0,I,0,I, 0,I,0,I, 0,I,0,I, 0,I,0,I, 0,I,0,I);

//-------------------------------------------------------------------------------------------------

int main()
    {
    txCreateWindow (800, 600);
    Win32::_fpreset();
    txBegin();

    scr_t front = (scr_t) LoadImage ("racket.bmp");
    scr_t back  = (scr_t) LoadImage ("table.bmp");
    scr_t scr   = (scr_t) *txVideoMemory();

    for (int n = 0; ; n++)
        {
        if (GetAsyncKeyState (VK_ESCAPE)) break;

        if (!GetKeyState (VK_CAPITAL))
            {
            for (int y = 0; y < 600; y++)
            for (int x = 0; x < 800; x += 8)
                {

                __m256i fr = _mm256_load_si256 ((__m256i*) &front[y][x]);                 //dst[127:0] := MEM[mem_addr+127:mem_addr]             // fr = front[y][x]
                __m256i bk = _mm256_load_si256 ((__m256i*) &back [y][x]);



                static const __m256i moveMask_fr_bk = _mm256_set_epi8 (Z, 15, Z, 14, Z, 13, Z, 12, Z, 11, Z, 10, Z, 9, Z, 8,
                                                                    Z, 7,  Z, 6,  Z, 5,  Z, 4,  Z, 3,  Z, 2,  Z, 1, Z, 0);

                static const __m256i moveMask_FR_BK = _mm256_set_epi8 (Z, 31, Z, 30, Z, 29, Z, 28, Z, 27, Z, 26, Z, 25, Z, 24,
                                                                    Z, 23, Z, 22, Z, 21, Z, 20, Z, 19, Z, 18, Z, 17, Z, 16);


                fr = _mm256_shuffle_epi8 (fr, moveMask_fr_bk);
                __m256i FR = _mm256_shuffle_epi8 (FR, moveMask_FR_BK);


                bk = _mm256_shuffle_epi8 (bk, moveMask_fr_bk);
                __m256i BK = _mm256_shuffle_epi8 (BK, moveMask_FR_BK);



                static const __m256i moveMask_a     = _mm256_set_epi8 (Z, 15, Z, 15, Z, 15, Z, 15, Z, 11, Z, 11, Z, 11, Z, 11,
                                                                    Z, 7,  Z, 7,  Z, 7,  Z, 7,  Z, 3,  Z, 3,  Z, 3,  Z, 3);

                static const __m256i moveMask_A     = _mm256_set_epi8 (Z, 31, Z, 31, Z, 31, Z, 31, Z, 27, Z, 27, Z, 27, Z, 27,
                                                                    Z, 23, Z, 23, Z, 23, Z, 23, Z, 19, Z, 19, Z, 19, Z, 19);

                __m256i a = _mm256_shuffle_epi8 (fr, moveMask_a);
                __m256i A = _mm256_shuffle_epi8 (FR, moveMask_A);


                //-----------------------------------------------------------------------

                fr = _mm256_mullo_epi16 (fr, a);                                                    // fr *= a
                FR = _mm256_mullo_epi16 (FR, A);

                bk = _mm256_mullo_epi16 (bk, _mm256_sub_epi16 (_255, a));                           // bk *= (255-a)
                BK = _mm256_mullo_epi16 (BK, _mm256_sub_epi16 (_255, A));

                __m256i sum = _mm256_add_epi16 (fr, bk);                                            // sum = fr*a + bk*(255-a)
                __m256i SUM = _mm256_add_epi16 (FR, BK);

                //-----------------------------------------------------------------------

                static const __m256i moveMask_sum = _mm256_set_epi8 (Z, Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z, Z, Z, Z, Z,
                                                                     31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1);
                static const __m256i moveMask_SUM = _mm256_set_epi8 (31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1,
                                                                     Z, Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z,  Z, Z, Z, Z, Z);
                sum = _mm256_shuffle_epi8 (sum, moveMask_sum);                                       // sum[i] = (sium[i] >> 8) = (sum[i] / 256)
                SUM = _mm256_shuffle_epi8 (SUM, moveMask_SUM);

                //-----------------------------------------------------------------------

                static const __m256i moveMask_clr = _mm256_set_epi8 (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


                __m256i color = (__m256i) _mm256_blendv_epi8 (sum, SUM, moveMask_clr);  // color = (sumHi << 8*8) | sum
                                                                                        //Move the lower 2 single-precision (32-bit) floating-point elements from b to the upper 2 elements of dst, and copy the lower 2 elements from a to the lower 2 elements of dst.

                //p128_hex_u8(color);

                _mm256_store_si256 ((__m256i*) &scr[y][x], color);                       //Store 128-bits of integer data from a into memory. mem_addr does not need to be aligned on any particular boundary.

                }
            }
        else
            {
            for (int y = 0; y < 600; y++)
            for (int x = 0; x < 800; x++)
                {
                RGBQUAD* fr = &front[y][x];
                RGBQUAD* bk = &back [y][x];

                uint16_t a  = fr->rgbReserved;

                if(a == 255) {scr[y][x]   = { (BYTE) (0), (BYTE) (0),(BYTE) (0) };}

                else {scr[y][x]   = { (BYTE) ( (fr->rgbBlue  * (a) + bk->rgbBlue  * (255-a)) >> 8 ),
                                (BYTE) ( (fr->rgbGreen * (a) + bk->rgbGreen * (255-a)) >> 8 ),
                                (BYTE) ( (fr->rgbRed   * (a) + bk->rgbRed   * (255-a)) >> 8 ) };}
                }
            }

        if (!(n % 10)) printf ("\t\r%.0lf", txGetFPS() * 10);
        txUpdateWindow();
        }

    txDisableAutoPause();
    }



