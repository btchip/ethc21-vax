/*
*******************************************************************************
*   Vax App PoC 
*   (c) 2021 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*   limitations under the License.
********************************************************************************/

#ifdef FUZZ
#include <stdint.h>
#else
#include "os.h"
#endif
#include "cbip_encode.h"
#include "cbip_decode.h"

// cbipTrue or cbipFalse
#define CBIPH_TYPE_BOOLEAN 0xfe
// cbipInt or cbipNegativeInt
#define CBIPH_TYPE_INT 0xff

#define CBIPH_STATUS_NOT_FOUND 0
#define CBIPH_ERROR_INVALID -1
#define CBIPH_ERROR_WRONG_TYPE -2
#define CBIPH_ERROR_MISSING_TYPE -3

int cbiph_validate(uint8_t *buffer, uint32_t length);

// go to the next field, supports map & arrays
int cbiph_next_deep(cbipDecoder_t *decoder, cbipItem_t *item);

/**
 * Find the item associated with a key (integer or string) in a map
 *
 * If stringKey is not NULL, the keys of the map are all expected to be of type
 * cbipTextString and the function returns the item whose key matches.
 *
 * Otherwise, the keys of the map are all expected to be of type cbipInt or
 * cbipNegativeInt and the function returns the item whose key matches the
 * parameter key.
 *
 * Return:
 * * 1 if the item was found
 * * CBIPH_STATUS_NOT_FOUND = 0 if the item was not found
 * * CBIPH_ERROR_... < 0 if an error occured
 */
int cbiph_get_map_item(cbipDecoder_t *decoder, cbipItem_t *mapItem, int key, const char *stringKey, cbipItem_t *keyItem, uint8_t checkType);

#ifdef HAVE_CBOR_DEBUG
void cbiph_dump(uint8_t *buffer, uint32_t length);
#endif
