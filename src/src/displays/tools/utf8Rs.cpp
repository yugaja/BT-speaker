#include "Arduino.h"
#include "../../core/options.h"
#if L10N_LANGUAGE == SRB
#include "../dspcore.h"
#include "utf8To.h"

/**
 * @brief Converts UTF-8 Serbian Latin text into pure ASCII.
 *        Serbian diacritics are transliterated:
 *        č ć → c
 *        š → s
 *        ž → z
 *        đ → dj
 *
 * @param str Input UTF-8 encoded string.
 * @param uppercase If true, ASCII output is converted to uppercase.
 * @return Pointer to static ASCII string buffer.
 *
 * @warning Returned buffer is static and will be overwritten on next call.
 */
char* utf8To(const char* str, bool uppercase)
{
    static char out[BUFLEN];
    size_t outIdx = 0;

    for (size_t i = 0; str[i] && outIdx < BUFLEN - 1; )
    {
        unsigned char c1 = (unsigned char)str[i];

        /* ASCII */
        if (c1 < 0x80)
        {
            out[outIdx++] = uppercase ? toupper(c1) : c1;
            i++;
            continue;
        }

        /* UTF-8 Serbian letters */
        if (str[i + 1])
        {
            unsigned char c2 = (unsigned char)str[i + 1];

            /* č ć */
            if (c1 == 0xC4 && (c2 == 0x8D || c2 == 0x87))
            {
                out[outIdx++] = uppercase ? 'C' : 'c';
                i += 2;
                continue;
            }

            /* Č Ć */
            if (c1 == 0xC4 && (c2 == 0x8C || c2 == 0x86))
            {
                out[outIdx++] = 'C';
                i += 2;
                continue;
            }

            /* š */
            if (c1 == 0xC5 && c2 == 0xA1)
            {
                out[outIdx++] = uppercase ? 'S' : 's';
                i += 2;
                continue;
            }

            /* Š */
            if (c1 == 0xC5 && c2 == 0xA0)
            {
                out[outIdx++] = 'S';
                i += 2;
                continue;
            }

            /* ž */
            if (c1 == 0xC5 && c2 == 0xBE)
            {
                out[outIdx++] = uppercase ? 'Z' : 'z';
                i += 2;
                continue;
            }

            /* Ž */
            if (c1 == 0xC5 && c2 == 0xBD)
            {
                out[outIdx++] = 'Z';
                i += 2;
                continue;
            }

            /* đ */
            if (c1 == 0xC4 && c2 == 0x91 && outIdx < BUFLEN - 2)
            {
                out[outIdx++] = uppercase ? 'D' : 'd';
                out[outIdx++] = uppercase ? 'J' : 'j';
                i += 2;
                continue;
            }

            /* Đ */
            if (c1 == 0xC4 && c2 == 0x90 && outIdx < BUFLEN - 2)
            {
                out[outIdx++] = 'D';
                out[outIdx++] = 'j';
                i += 2;
                continue;
            }
        }

        /* Unknown UTF-8 sequence */
        out[outIdx++] = '?';
        i++;
    }

    out[outIdx] = '\0';
    return out;
}


#endif
