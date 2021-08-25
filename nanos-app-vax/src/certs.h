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

#include <stdint.h>

typedef struct signData_t {
  uint8_t kid[8];
  uint8_t key[65];
} signData_t;

#if 0
#define NUM_CERTS 1
#else
#define NUM_CERTS 140
#endif

extern signData_t const CERTS[NUM_CERTS];
