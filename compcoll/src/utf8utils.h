/**
 * @file    utf8utils.h
 * @brief   utf8 functions
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @license GPL 2.0/LGPL 2.1
 * @date    2016-03-07
 */
#ifndef __MY_UTF8FUNC_H
#define __MY_UTF8FUNC_H

#include <stdint.h>    /* uint8_t */
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

uint8_t * get_next_utf8_pos (uint8_t *pstart);
wchar_t get_val_utf82uni (uint8_t *pstart);
uint8_t * get_utf8_value (uint8_t *pstart, wchar_t *pval);
int uni_to_utf8 (size_t val, uint8_t *buf, size_t szbuf);

#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /* __MY_UTF8FUNC_H */
