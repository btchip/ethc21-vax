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

#include "os.h"
#include "cbip_decode.h"

typedef struct certInfo_t {

	int        startDate;
	int        endDate;
	cbipItem_t payloadItem;
	cbipItem_t firstNameStd;
	cbipItem_t lastNameStd;
	uint8_t		 id[32];

} certInfo_t;

#define MAX_HC_BUFFER 500
#define MAX_SALT_BUFFER 20

typedef struct contextData_t {

	uint8_t hcBuffer[MAX_HC_BUFFER];
	uint16_t hcOffset;
	uint16_t hcLength;
	uint8_t saltBuffer[MAX_SALT_BUFFER];
	uint8_t saltBufferLength;
	uint8_t ethAddress[20];
	certInfo_t certInfos;

} contextData_t;

extern contextData_t contextData;
