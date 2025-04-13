/* Network utils header

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Start wifi scan
 */
void wifi_start_scan();

/**
 * @brief  Get names of availabe APs separated by null terminator
 *
 * @param[out]  buffer  Buffer for allocated string
 *
 * @return
 *      - <=0  On failure
 *      - >0   Number of strings
 */
int get_available_networks(char **buffer);

#ifdef __cplusplus
}
#endif

#endif
