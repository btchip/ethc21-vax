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

#include "hcParser.h"
#include "cbip_decode.h"
#include "cbip_helper.h"
#include "certs.h"
#include "sharedData.h"
#include "ui.h"
#include <string.h>

const char SIGNATURE1_PREFIX[] = "Signature1";
const char TG_COVID[] = "840539006";

void reportError(char *errorText, volatile unsigned int *tx) {
	G_io_apdu_buffer[0] = 0x00;
	memmove(G_io_apdu_buffer + 1, errorText, strlen(errorText));
	*tx = strlen(errorText) + 1;
	THROW(SW_OK);
}

void getNextHcBodyItem(cbipDecoder_t *decoder, cbipItem_t *item, volatile unsigned int *tx) {
	int status;	
	status = cbip_next(decoder, item);
	if (status != 0) {
		reportError("getNextHcBodyItem next failed", tx);
	}
	if (item->type != cbipByteString) {
		reportError("Invalid body item type", tx);
	}
}

void getCert(uint8_t *kid, uint32_t kidLength, cx_ecfp_public_key_t *publicKey, volatile unsigned int *tx) {
	uint32_t i;
	if (kidLength != 8) {
		reportError("Invalid key id length", tx);		
	}
	for (i=0; i<NUM_CERTS; i++) {
		if (memcmp(CERTS[i].kid, kid, kidLength) == 0) {
			cx_ecfp_init_public_key(CX_CURVE_SECP256R1, CERTS[i].key, 65, publicKey);
			break;
		}
	}
	if (i == NUM_CERTS) {
		reportError("Unknown key id", tx);
	}
}

void parseHcHeader(uint8_t *hcBuffer, cbipItem_t *headerItem, cx_ecfp_public_key_t *publicKey, volatile unsigned int *tx) {
	int status;
	cbipDecoder_t decoder;
	cbipItem_t mapItem, item;
	status = cbip_decoder_init(&decoder, hcBuffer + headerItem->offset + headerItem->headerLength, headerItem->value);
	if (status != 0) {
		reportError("parseHcHeader decoder init failed", tx);
	}
	status = cbip_first(&decoder, &mapItem);
	if (status != 0) {
		reportError("parseHcHeader cbip_first failed", tx);
	}
	if (mapItem.type != cbipMap)	{
		reportError("parseHcHeader invalid header type", tx);
	}
	status = cbiph_get_map_item(&decoder, &mapItem, 4, NULL, &item, cbipByteString);
	if (status != 1) {
		reportError("parseHcHeader kid not found", tx);
	}
	PRINTF("Kid %.*H\n", item.value, decoder.buffer + item.offset + item.headerLength);
	getCert(decoder.buffer + item.offset + item.headerLength, item.value, publicKey, tx);
}

void signatureToDER(uint8_t *signature, uint8_t *out, uint32_t *signatureLength) {
	uint8_t offset = 0;
	out[offset++] = 0x30;
	out[offset++] = 0x44 + ((signature[0] & 0x80) ? 1 : 0) + ((signature[32] & 0x80) ? 1 : 0);
	out[offset++] = 0x02;
	out[offset++] = 0x20 + ((signature[0] & 0x80) ? 1 : 0);
	if (signature[0] & 0x80) {
		out[offset++] = 0x00;
	}
	memmove(out + offset, signature, 32);
	offset += 32;
	out[offset++] = 0x02;
	out[offset++] = 0x20 + ((signature[32] & 0x80) ? 1 : 0);
	if (signature[32] & 0x80) {
		out[offset++] = 0x00;
	}
	memmove(out + offset, signature + 32, 32);
	offset += 32;
	*signatureLength = offset;
}

void checkHcSignature(uint8_t *hcBuffer, cbipItem_t *headerItem, cbipItem_t *payloadItem, cbipItem_t *signatureItem, cx_ecfp_public_key_t *publicKey, volatile unsigned int *tx) {
	int status;
	uint8_t tmp[32];
	uint8_t signature[72];
	uint32_t signatureLength;
	cbipEncoder_t encoder;
	cx_sha256_t sha256;

	cx_sha256_init(&sha256);
	status = cbip_encoder_init(&encoder, tmp, sizeof(tmp));
	if (status != 0) {
		THROW(EXCEPTION);
	}
	status = cbip_add_array_header(&encoder, 4);
	if (status != 0) {
		THROW(EXCEPTION);
	}	
	status = cbip_add_string(&encoder, SIGNATURE1_PREFIX);
	if (status != 0) {
		THROW(EXCEPTION);
	}
	status = cbip_add_byte_string(&encoder, hcBuffer + headerItem->offset + headerItem->headerLength, headerItem->value);
	if (status != 0) {
		THROW(EXCEPTION);
	}
	status = cbip_add_byte_string(&encoder, tmp, 0);
	if (status != 0) {
		THROW(EXCEPTION);
	}
	PRINTF("Data hashed %.*H\n", encoder.offset, tmp);
	cx_hash((cx_hash_t*)&sha256, 0, tmp, encoder.offset, NULL, 0);
	PRINTF("Data hashed %.*H\n", payloadItem->value + payloadItem->headerLength, hcBuffer + payloadItem->offset);
	cx_hash((cx_hash_t*)&sha256, CX_LAST, hcBuffer + payloadItem->offset, payloadItem->value + payloadItem->headerLength, tmp, 32);
	PRINTF("Hash %.*H\n", 32, tmp);
	signatureToDER(hcBuffer + signatureItem->offset + signatureItem->headerLength, signature, &signatureLength);
	PRINTF("Signature %.*H\n", signatureLength, signature);
	PRINTF("Key %.*H\n", 65, publicKey->W);
	status = cx_ecdsa_verify(publicKey, CX_LAST, CX_NONE, tmp, 32, signature, signatureLength);
	if (!status) {
		reportError("Signature not verified", tx);
	}
}

void parsePayload(uint8_t *hcBuffer, cbipItem_t *payloadItem, certInfo_t *certInfo, volatile unsigned int *tx) {
	int status;
	cbipDecoder_t decoder;
	cbipItem_t mapItem, mapItemV, item;
	int maxDoses, currentDoses;	
	PRINTF("parsePayload\n");
	status = cbip_decoder_init(&decoder, hcBuffer + payloadItem->offset + payloadItem->headerLength, payloadItem->value);
	if (status != 0) {
		reportError("parsePayload decoder init failed", tx);
	}
	status = cbip_first(&decoder, &mapItem);
	if (status != 0) {
		reportError("parsePayload cbip_first failed", tx);
	}
	if (mapItem.type != cbipMap)	{
		reportError("parsePayload invalid payload type", tx);
	}
	status = cbiph_get_map_item(&decoder, &mapItem, 6, NULL, &item, cbipInt);
	if (status != 1) {
		reportError("Issuing date not found", tx);
	}
	certInfo->startDate = cbip_get_int(&item);
	status = cbiph_get_map_item(&decoder, &mapItem, 4, NULL, &item, cbipInt);
	if (status != 1) {
		reportError("Expiring date not found", tx);
	}
	certInfo->endDate = cbip_get_int(&item);
	status = cbiph_get_map_item(&decoder, &mapItem, -260, NULL, &item, cbipMap);
	if (status != 1) {
		reportError("Payload not found", tx);
	}
	memmove((void*)&mapItem, (void*)&item, sizeof(cbipItem_t));
	// Locate the vaccine information
	status = cbiph_get_map_item(&decoder, &mapItem, 1, NULL, &item, cbipMap);
	if (status != 1) {
		reportError("Payload content not found", tx);
	}
	memmove((void*)&mapItem, (void*)&item, sizeof(cbipItem_t));
	status = cbiph_get_map_item(&decoder, &mapItem, 0, "v", &mapItemV, cbipArray);
	if (status != 1) {
		reportError("Vaccine information not found", tx);
	}
	if (mapItemV.value != 1) {
		reportError("Invalid vaccine array length", tx);
	}
	status = cbip_next(&decoder, &mapItemV);
	if (status != 0) {
		reportError("Vaccine cbip_next failed", tx);
	}
	if (mapItemV.type != cbipMap) {
		reportError("Invalid vaccine structure", tx);
	}
	// Verify the vaccine information
	status = cbiph_get_map_item(&decoder, &mapItemV, 0, "tg", &item, cbipNone);
	if (status != 1) {
		reportError("Target disease not found", tx);
	}
	if ((item.value != strlen(TG_COVID)) || 
	   	(memcmp(decoder.buffer + item.offset + item.headerLength, TG_COVID, strlen(TG_COVID)) != 0)) {
		reportError("Invalid target disease", tx);
	}
	status = cbiph_get_map_item(&decoder, &mapItemV, 0, "sd", &item, cbipInt);
	if (status != 1) {
		reportError("Max number of doses not found", tx);
	}	
	maxDoses = cbip_get_int(&item);
	status = cbiph_get_map_item(&decoder, &mapItemV, 0, "dn", &item, cbipInt);
	if (status != 1) {
		reportError("Current number of doses not found", tx);
	}	
	currentDoses = cbip_get_int(&item);
	if (currentDoses < maxDoses) {
		reportError("Vaccination scheme not finalized", tx);
	}
	// Extract personal information
	status = cbiph_get_map_item(&decoder, &mapItem, 0, "nam", &mapItemV, cbipMap);
	if (status != 1) {
		reportError("Name information not found", tx);
	}
	status = cbiph_get_map_item(&decoder, &mapItemV, 0, "gnt", &certInfo->firstNameStd, cbipTextString);
	if (status != 1) {
		reportError("Standardized first name not found", tx);
	}	
	status = cbiph_get_map_item(&decoder, &mapItemV, 0, "fnt", &certInfo->lastNameStd, cbipTextString);
	if (status != 1) {
		reportError("Standardized last name not found", tx);
	}	
	// Compute certificate ID
	cx_hash_sha256(hcBuffer + payloadItem->offset + payloadItem->headerLength, payloadItem->value, certInfo->id, sizeof(certInfo->id));
	PRINTF("start date %d\n", certInfo->startDate);
	PRINTF("end date %d\n", certInfo->endDate);
	PRINTF("first name %.*H\n", certInfo->firstNameStd.value, decoder.buffer + certInfo->firstNameStd.offset + certInfo->firstNameStd.headerLength);
	PRINTF("last name %.*H\n", certInfo->lastNameStd.value, decoder.buffer + certInfo->lastNameStd.offset + certInfo->lastNameStd.headerLength);	
	PRINTF("ID %.*H\n", sizeof(certInfo->id), certInfo->id);
}

void parseHc(uint8_t *hcBuffer, uint16_t hcLength, volatile unsigned int *flags, volatile unsigned int *tx) {
	int status;
	cbipDecoder_t decoder;
	cbipItem_t headerItem, signatureItem, item;
	cx_ecfp_public_key_t publicKey;
	// Decode all components and verify the signature
	PRINTF("Length %d\n", hcLength);
	status = cbip_decoder_init(&decoder, hcBuffer, hcLength);
	if (status != 0) {
		reportError("parseHc decoder init failed", tx);
	}
	status = cbip_first(&decoder, &item);
	if (status != 0) {
		reportError("parseHc cbip_first failed", tx);
	}
	if (item.type == cbipTag) {
		status = cbip_next(&decoder, &item);
		if (status != 0) {
			reportError("parseHc cbip_next tag failed", tx);
		}
	}
	if (item.type != cbipArray) {
		reportError("Invalid structure start type", tx);
	}
	if (item.value != 4) {
		reportError("parseHc invalid array length", tx);
	}
	getNextHcBodyItem(&decoder, &item, tx);
	memmove((void*)&headerItem, (void*)&item, sizeof(cbipItem_t));
	// Ignore unprotected header
	status = cbip_next(&decoder, &item);
	if (status != 0) {
		reportError("parseHc cbip_next unprotected header failed", tx);
	}
	getNextHcBodyItem(&decoder, &item, tx);
	memmove((void*)&contextData.certInfos.payloadItem, (void*)&item, sizeof(cbipItem_t));
	getNextHcBodyItem(&decoder, &item, tx);
	memmove((void*)&signatureItem, (void*)&item, sizeof(cbipItem_t));
	parseHcHeader(hcBuffer, &headerItem, &publicKey, tx);
	checkHcSignature(hcBuffer, &headerItem, &contextData.certInfos.payloadItem, &signatureItem, &publicKey, tx);
	parsePayload(hcBuffer, &contextData.certInfos.payloadItem, &contextData.certInfos, tx);

	ux_flow_init(0, ux_hc_receipt, NULL);
	*flags |= IO_ASYNCH_REPLY;
}
