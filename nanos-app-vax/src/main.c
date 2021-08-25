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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"
#include "ux.h"

#include "os_io_seproxyhal.h"

#include "glyphs.h"

#include "hcParser.h"
#include "sharedData.h"
#include "ui.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

#define CLA 0xE0

#define INS_PROCESS_HC 0x02
#define INS_GET_ENDORSEMENT_DATA 0x04

#define P1_HC_START 0x00
#define P1_HC_NEXT 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

bagl_element_t tmp_element;

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

contextData_t contextData;

//////////////////////////////////////////////////////////////////////
UX_FLOW_DEF_NOCB(
    ux_idle_flow_1_step,
    nn, //pnn,
    {
      //"", //&C_icon_dashboard,
      "Application",
      "is ready",
    });
UX_FLOW_DEF_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
      "Version",
      APPVERSION,
    });
UX_FLOW_DEF_VALID(
    ux_idle_flow_3_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard_x,
      "Quit",
    });
const ux_flow_step_t *        const ux_idle_flow [] = {
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  FLOW_END_STEP,
};

void ui_idle(void) {
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; // nothing received from the master so far (it's a tx
                      // transaction)
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

void handleProcessHc(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  uint8_t dataOffset = 0;
  if (p1 == P1_HC_START) {
    if (dataLength < 2 + 1 + 20 + dataBuffer[2]) {
      PRINTF("Data too small\n");
      THROW(SW_WRONG_DATA);
    }
    contextData.hcLength = U2BE(dataBuffer, 0);
    if (contextData.hcLength >= MAX_HC_BUFFER) {
      PRINTF("Data too large\n");
      contextData.hcLength = 0;
      THROW(SW_WRONG_DATA);
    }
    contextData.saltBufferLength = dataBuffer[2];
    if (contextData.saltBufferLength >= MAX_SALT_BUFFER) {
      PRINTF("Salt too large\n");
      contextData.hcLength = 0;
      THROW(SW_WRONG_DATA);
    }
    memmove(contextData.saltBuffer, dataBuffer + 3, contextData.saltBufferLength);
    memmove(contextData.ethAddress, dataBuffer + 3 + contextData.saltBufferLength, 20);
    dataOffset = 3 + contextData.saltBufferLength + 20;
    contextData.hcOffset = 0;
  }
  else if (p1 != P1_HC_NEXT) {
    THROW(SW_INCORRECT_P1_P2);
  }
  if (((contextData.hcOffset + dataLength - dataOffset) > contextData.hcLength) || ((contextData.hcOffset + dataLength - dataOffset) <= contextData.hcOffset)) {
    PRINTF("Message too large\n");
    THROW(SW_WRONG_DATA);
  }
  memmove(contextData.hcBuffer + contextData.hcOffset, dataBuffer + dataOffset, dataLength - dataOffset);
  contextData.hcOffset += dataLength - dataOffset;
  if (contextData.hcOffset == contextData.hcLength) {
    parseHc(contextData.hcBuffer, contextData.hcLength, flags, tx);
  }
  else {
    THROW(SW_OK);
  }
}

void handleGetEndorsementData(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
  uint8_t offset = 0;
  os_endorsement_get_public_key(1, G_io_apdu_buffer);
  offset += 65;
  os_endorsement_get_public_key_certificate(1, G_io_apdu_buffer + offset);
  *tx = offset + G_io_apdu_buffer[offset + 1] + 2;
  THROW(0x9000);
}


void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
  unsigned short sw = 0;

  BEGIN_TRY {
    TRY {

      if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
        THROW(0x6E00);
      }

      switch (G_io_apdu_buffer[OFFSET_INS]) {
        case INS_PROCESS_HC:
          handleProcessHc(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;
        case INS_GET_ENDORSEMENT_DATA:
          handleGetEndorsementData(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
          break;

#if 0
        case 0xFF: // return to dashboard
          goto return_to_dashboard;
#endif

        default:
          THROW(0x6D00);
          break;
      }
    }
    CATCH(EXCEPTION_IO_RESET) {
      THROW(EXCEPTION_IO_RESET);
    }
    CATCH_OTHER(e) {
      switch (e & 0xF000) {
        case 0x6000:
          // Wipe the transaction context and report the exception
          sw = e;
          break;
        case 0x9000:
          // All is well
          sw = e;
          break;
        default:
          // Internal error
          sw = 0x6800 | (e & 0x7FF);
          break;
        }
        // Unexpected exception => report
        G_io_apdu_buffer[*tx] = sw >> 8;
        G_io_apdu_buffer[*tx + 1] = sw;
        *tx += 2;
      }
      FINALLY {
      }
  }
  END_TRY;
}


void sample_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
              THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                if (e != 0x9000) {
                    flags &= ~IO_ASYNCH_REPLY;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
  io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
    		UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
    		break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
         THROW(EXCEPTION_IO_RESET);
        }
        // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer,
        {
        });
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {

    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {

        }
    }
    END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(int arg0) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

#ifdef TARGET_NANOX

                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);

#endif                

#if 0
                if (N_storage.initialized != 0x01) {
                  internalStorage_t storage;
                  storage.dataAllowed = 0x00;
                  storage.contractDetails = 0x00;
                  storage.initialized = 0x01;
                  nvm_write(&N_storage, (void*)&storage, sizeof(internalStorage_t));
                }
                dataAllowed = N_storage.dataAllowed;
                contractDetails = N_storage.contractDetails;
#endif                

                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

                contextData.hcLength = 0;
                contextData.hcOffset = 0;


                sample_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
              // reset IO and UX before continuing
              continue;
            }
            CATCH_ALL {
              break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
	  app_exit();
    return 0;
}
