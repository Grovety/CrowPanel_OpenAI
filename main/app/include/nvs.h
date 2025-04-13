/* Non-volatile storage utils header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _NVS_H_
#define _NVS_H_

/**
 * @brief  Init esp nvs storage
 */
int nvs_init();

/**
 * @brief  Write key-value string to partition
 *
 * @param[in]  part  Partition namespace
 * @param[in]  key   Key string
 * @param[in]  str   Value string
 *
 * @return
 *      - 0  On failure
 *      - 1  On success
 */
int nvs_store_str(const char *part, const char *key, const char *str);

/**
 * @brief  Read value by key from partition
 *
 * @param[in]  part     Partition namespace
 * @param[in]  key      Key string
 * @param[out] ret_buf  Pointer to allocated buffer
 *
 * @return
 *      - 0  On failure
 *      - 1  On success
 */
int nvs_load_str(const char *part, const char *key, char **ret_buf);

/**
 * @brief  Clear partition storage
 *
 * @param[in]  part  Partition namespace
 *
 * @return
 *      - 0  On failure
 *      - 1  On success
 */
int nvs_erase_data(const char *part);

#endif
