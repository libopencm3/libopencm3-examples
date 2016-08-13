/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2016 Oliver Meier <h2obrain@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef UTF8_H_
#define UTF8_H_

#ifndef NULL
#define NULL 0
#endif

#include <stdint.h>

/**
 *
 * @param text    utf8 string
 * @param value   pointer which holds the utf value after the function returns
 * @return        pointer to the next value in string (-1 for invalid value)
 */
static inline
const char *
utf8_read_value(const char *text, int32_t *value)
{
	char tmp; uint8_t cnt, i;

	tmp = *text++;

	if (!(tmp & 0b10000000)) {
		*value = tmp;
		return text;
	} else
	if  ((tmp & 0b11100000) == 0b11000000) {
		*value = tmp & ~0b11100000;
		cnt = 1;
	} else
	if  ((tmp & 0b11110000) == 0b11100000) {
		*value = tmp & ~0b11110000;
		cnt = 2;
	} else
	if  ((tmp & 0b11111000) == 0b11110000) {
		*value = tmp & ~0b11111000;
		cnt = 3;
	} else {
		*value = -1;
		return text;
	}

	for (i = cnt; i; i--) {
		tmp = *text++;
		if  ((tmp & 0b11000000) != 0b10000000) {
			*value = -1;
			return text;
		}

		*value = (*value<<6) | (tmp & ~0b11000000);
	}

	return text;
}


/* utf-8 ... */
static inline
const char *
utf8_find_character_index(
		int index,
		const char *text_start, const char *text_end,
		int *index_rest
) {
	const char *result;
	if (index < 0) {
		result = text_end;
		while (index                       && result != text_start) {
			index++;
			result--;
			/* skip next utf8-character (secondary bytes) */
			while (((*result)&192) == 128  && result != text_start) {
				result--;
			}
		}
	} else {
		result = text_start;
		while (index                       && result != text_end) {
			index--;
			result++;
			/* skip last utf8-character (secondary bytes) */
			while (((*result)&192) == 128  && result != text_end) {
				result++;
			}
		}
	}

	if (index_rest != NULL) {
		*index_rest = index;
	}

	return result;
}


/* untestet! */
static inline
const char *utf8_find_character_in_string(
		const char chr,
		const char *text_start, const char *text_end
		 /*, int *index_rest */
) {
	const char *result = text_start;
	while (result != text_end) {
		if (*result == chr) {
			break;
		}
		result = utf8_find_character_index(1, result, text_end, NULL);
	}
	return result;
}
static inline
int utf8_find_pointer_diff(const char *text_start, const char *text_end)
{
	int diff = 0;
	while (text_start < text_end) {
		if (!*text_start) {
			return -1;
		}

		text_start++;
		diff++;
		/* skip last utf8-character (secondary bytes) */
		while (((*text_start)&192) == 128  && text_start < text_end) {
			text_start++;
		}
	}
	return diff;
}


#endif
