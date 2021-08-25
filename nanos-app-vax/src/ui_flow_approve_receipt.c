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
#include "ux.h"
#include "sharedData.h"
#include "ui.h"
#include <string.h>

char tmp[100];

void copy_string_item(cbipItem_t *item) {
	uint8_t *buffer = contextData.hcBuffer + contextData.certInfos.payloadItem.offset + contextData.certInfos.payloadItem.headerLength;
	uint8_t size = MIN(sizeof(tmp) - 1, item->value);
	memmove(tmp, buffer + item->offset + item->headerLength, size);
	tmp[size] = '\0';
}

void hc_receipt_display_first_name() {
	copy_string_item(&contextData.certInfos.firstNameStd);
}

void hc_receipt_display_last_name() {
	copy_string_item(&contextData.certInfos.lastNameStd);
}

void hc_receipt_display_privacy_salt() {
	memmove(tmp, contextData.saltBuffer, contextData.saltBufferLength);
	tmp[contextData.saltBufferLength] = '\0';
}

void hc_receipt_display_eth_address() {
	snprintf(tmp, sizeof(tmp), "0x%.*H", 20, contextData.ethAddress);
}

UX_STEP_NOCB(
		ux_hc_receipt_1_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "DGC receipt",
    });

UX_STEP_NOCB_INIT(
    ux_hc_receipt_2_step,
    bnnn_paging,
    hc_receipt_display_first_name(),
    {
      .title = "First name",
      .text = tmp
    });

UX_STEP_NOCB_INIT(
    ux_hc_receipt_3_step,
    bnnn_paging,
    hc_receipt_display_last_name(),
    {
      .title = "Last name",
      .text = tmp
    });

UX_STEP_NOCB_INIT(
    ux_hc_receipt_4_step,
    bnnn_paging,
    hc_receipt_display_privacy_salt(),
    {
      .title = "Privacy salt",
      .text = tmp
    });

UX_STEP_NOCB_INIT(
    ux_hc_receipt_5_step,
    bnnn_paging,
    hc_receipt_display_eth_address(),
    {
      .title = "ETH address",
      .text = tmp
    });

UX_STEP_CB(
    ux_hc_receipt_6_step,
    pbb,
    hc_receipt_approve(NULL),
    {
      &C_icon_validate_14,
      "Generate",
      "receipt",
    });
UX_STEP_CB(
    ux_hc_receipt_7_step,
    pb,
    hc_receipt_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_hc_receipt,
	&ux_hc_receipt_1_step,
	&ux_hc_receipt_2_step,
	&ux_hc_receipt_3_step,
	&ux_hc_receipt_4_step,
	&ux_hc_receipt_5_step,
	&ux_hc_receipt_6_step,
	&ux_hc_receipt_7_step);

