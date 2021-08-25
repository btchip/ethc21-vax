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
#include "cx.h"
#include "ux.h"
#include "sharedData.h"
#include "ui.h"
#include <string.h>

unsigned int hc_receipt_approve(__attribute__((unused)) const bagl_element_t *e) {
	uint8_t offset = 0;
	cx_sha256_t sha256;
	uint8_t *buffer = contextData.hcBuffer + contextData.certInfos.payloadItem.offset + contextData.certInfos.payloadItem.headerLength;

	G_io_apdu_buffer[offset++] = 0x01;
	memmove(G_io_apdu_buffer + offset, contextData.certInfos.id, sizeof(contextData.certInfos.id));
	offset += sizeof(contextData.certInfos.id);
	memmove(G_io_apdu_buffer + offset, contextData.ethAddress, sizeof(contextData.ethAddress));
	offset += sizeof(contextData.ethAddress);
	cx_sha256_init(&sha256);
	cx_hash((cx_hash_t*)&sha256, 0, 
		buffer + contextData.certInfos.firstNameStd.offset + contextData.certInfos.firstNameStd.headerLength, 
		contextData.certInfos.firstNameStd.value, NULL, 0);
	cx_hash((cx_hash_t*)&sha256, 0, 
		buffer + contextData.certInfos.lastNameStd.offset + contextData.certInfos.lastNameStd.headerLength, 
		contextData.certInfos.lastNameStd.value, NULL, 0);
	cx_hash((cx_hash_t*)&sha256, CX_LAST, 
		contextData.saltBuffer, contextData.saltBufferLength,
		G_io_apdu_buffer + offset, 32);
	offset += 32;
	U4BE_ENCODE(G_io_apdu_buffer, offset, contextData.certInfos.startDate);
	offset += 4;
	U4BE_ENCODE(G_io_apdu_buffer, offset, contextData.certInfos.endDate);
	offset += 4;
	os_endorsement_get_code_hash(G_io_apdu_buffer + offset);
	offset += 32;
	os_endorsement_key1_sign_data(G_io_apdu_buffer, offset, G_io_apdu_buffer + offset);
	offset += G_io_apdu_buffer[offset + 1] + 2;
	G_io_apdu_buffer[offset++] = 0x90;
	G_io_apdu_buffer[offset++] = 0x00;
	// Send back the response, do not restart the event loop
	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, offset);
	// Display back the original UX
	ui_idle();

	return 0; // do not redraw the widget
}

unsigned int hc_receipt_cancel(__attribute__((unused)) const bagl_element_t *e) {
	G_io_apdu_buffer[0] = 0x69;
	G_io_apdu_buffer[1] = 0x85;
	// Send back the response, do not restart the event loop
	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
	// Display back the original UX
	ui_idle();

	return 0; // do not redraw the widget
}

