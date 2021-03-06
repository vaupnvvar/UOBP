/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#include "prologue.h"

#include <string.h>
#include <errno.h>

#include "log.h"
#include "io_generic.h"

#include "brl_driver.h"
#include "brldefs-np.h"

#define PROBE_RETRY_LIMIT 2
#define PROBE_INPUT_TIMEOUT 1000
#define MAXIMUM_RESPONSE_SIZE (0XFF + 4)
#define MAXIMUM_CELL_COUNT 140

BEGIN_KEY_NAME_TABLE(navigation)
  KEY_NAME_ENTRY(NP_KEY_PadLeft1, "PadLeft1"),
  KEY_NAME_ENTRY(NP_KEY_PadUp1, "PadUp1"),
  KEY_NAME_ENTRY(NP_KEY_PadCenter1, "PadCenter1"),
  KEY_NAME_ENTRY(NP_KEY_PadDown1, "PadDown1"),
  KEY_NAME_ENTRY(NP_KEY_PadRight1, "PadRight1"),

  KEY_NAME_ENTRY(NP_KEY_LeftUpper1, "LeftUpper1"),
  KEY_NAME_ENTRY(NP_KEY_LeftMiddle1, "LeftMiddle1"),
  KEY_NAME_ENTRY(NP_KEY_LeftLower1, "LeftLower1"),
  KEY_NAME_ENTRY(NP_KEY_RightUpper1, "RightUpper1"),
  KEY_NAME_ENTRY(NP_KEY_RightMiddle1, "RightMiddle1"),
  KEY_NAME_ENTRY(NP_KEY_RightLower1, "RightLower1"),

  KEY_NAME_ENTRY(NP_KEY_PadLeft2, "PadLeft2"),
  KEY_NAME_ENTRY(NP_KEY_PadUp2, "PadUp2"),
  KEY_NAME_ENTRY(NP_KEY_PadCenter2, "PadCenter2"),
  KEY_NAME_ENTRY(NP_KEY_PadDown2, "PadDown2"),
  KEY_NAME_ENTRY(NP_KEY_PadRight2, "PadRight2"),

  KEY_NAME_ENTRY(NP_KEY_LeftUpper2, "LeftUpper2"),
  KEY_NAME_ENTRY(NP_KEY_LeftMiddle2, "LeftMiddle2"),
  KEY_NAME_ENTRY(NP_KEY_LeftLower2, "LeftLower2"),
  KEY_NAME_ENTRY(NP_KEY_RightUpper2, "RightUpper2"),
  KEY_NAME_ENTRY(NP_KEY_RightMiddle2, "RightMiddle2"),
  KEY_NAME_ENTRY(NP_KEY_RightLower2, "RightLower2"),

  KEY_SET_ENTRY(NP_SET_RoutingKey, "RoutingKey"),
END_KEY_NAME_TABLE

BEGIN_KEY_NAME_TABLES(all)
  KEY_NAME_TABLE(navigation),
END_KEY_NAME_TABLES

DEFINE_KEY_TABLE(all)

BEGIN_KEY_TABLE_LIST
  &KEY_TABLE_DEFINITION(all),
END_KEY_TABLE_LIST

typedef struct {
  unsigned char identifier;
  unsigned char cellCount;
} ModelEntry;

static const ModelEntry modelTable[] = {
  { .identifier = 0X70,
    .cellCount = 0
  },

  { .identifier = 0X72,
    .cellCount = 20
  },

  { .identifier = 0X74,
    .cellCount = 40
  },

  { .identifier = 0X76,
    .cellCount = 60
  },

  { .identifier = 0X78,
    .cellCount = 80
  },

  { .identifier = 0X7A,
    .cellCount = 100
  },

  { .identifier = 0X7C,
    .cellCount = 120
  },

  { .identifier = 0X7E,
    .cellCount = 140
  },

  { .identifier = 0 }
};

struct BrailleDataStruct {
  GioEndpoint *gioEndpoint;
  const ModelEntry *model;
  int forceRewrite;
  int acknowledgementPending;
  unsigned char textCells[MAXIMUM_CELL_COUNT];
};

static const ModelEntry *
getModelEntry (unsigned char identifier) {
  const ModelEntry *model = modelTable;

  while (model->identifier) {
    if (identifier == model->identifier) return model;
    model += 1;
  }

  logMessage(LOG_WARNING, "unknown NinePoint model: 0X%02X", identifier);
  return NULL;
}

static int
setModel (BrailleDisplay *brl, unsigned char identifier) {
  const ModelEntry *model = getModelEntry(identifier);

  if (model) {
    logMessage(LOG_NOTICE, "NinePoint Model: 0X%02X, %u cells",
               model->identifier, model->cellCount);

    brl->data->model = model;
    brl->textColumns = model->cellCount;
    return 1;
  }

  return 0;
}

static int
writeBytes (BrailleDisplay *brl, const unsigned char *bytes, size_t count) {
  return writeBraillePacket(brl, brl->data->gioEndpoint, bytes, count);
}

static int
writePacket (BrailleDisplay *brl, unsigned char type, size_t size, const unsigned char *data) {
  unsigned char bytes[size + 5];
  unsigned char *byte = bytes;

  *byte++ = NP_PKT_BEGIN;
  *byte++ = brl->data->model->identifier;
  *byte++ = size + 1;
  *byte++ = type;
  byte = mempcpy(byte, data, size);
  *byte++ = NP_PKT_END;

  return writeBytes(brl, bytes, byte-bytes);
}

static size_t
readPacket (BrailleDisplay *brl, void *packet, size_t size) {
  unsigned char *bytes = packet;
  size_t offset = 0;
  size_t length = 0;

  while (1) {
    unsigned char byte;

    {
      int started = offset > 0;

      if (!gioReadByte(brl->data->gioEndpoint, &byte, started)) {
        if (started) logPartialPacket(bytes, offset);
        return 0;
      }
    }

  gotByte:
    if (offset == 0) {
      switch (byte) {
        case NP_RSP_Identity:
          length = 2;
          break;

        case NP_PKT_BEGIN:
          length = 3;
          break;

        default:
          logIgnoredByte(byte);
          continue;
      }
    } else {
      int unexpected = 0;

      switch (bytes[0]) {
        case NP_PKT_BEGIN:
          if (offset == 1) {
            if (byte != brl->data->model->identifier) {
              if (setModel(brl, byte)) {
                brl->resizeRequired = 1;
              } else {
                unexpected = 1;
              }
            }
          } else if (offset == 2) {
            length += byte + 1;
          } else if (offset == (length-1)) {
            if (byte != NP_PKT_END) unexpected = 1;
          }
          break;

        default:
          break;
      }

      if (unexpected) {
        logShortPacket(bytes, offset);
        offset = 0;
        length = 0;
        goto gotByte;
      }
    }

    if (offset < size) {
      bytes[offset] = byte;

      if (offset == (length - 1)) {
        logInputPacket(bytes, length);
        return length;
      }
    } else {
      if (offset == size) logTruncatedPacket(bytes, offset);
      logDiscardedByte(byte);
    }

    offset += 1;
  }
}

static int
connectResource (BrailleDisplay *brl, const char *identifier) {
  static const SerialParameters serialParameters = {
    SERIAL_DEFAULT_PARAMETERS,
    .baud = 19200,
    .parity = SERIAL_PARITY_ODD
  };

  static const UsbChannelDefinition usbChannelDefinitions[] = {
    { .vendor=0X0403, .product=0X6001, 
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=2,
      .serial = &serialParameters
    }
    ,
    { .vendor=0 }
  };

  GioDescriptor descriptor;
  gioInitializeDescriptor(&descriptor);

  descriptor.usb.channelDefinitions = usbChannelDefinitions;

  if ((brl->data->gioEndpoint = gioConnectResource(identifier, &descriptor))) {
    return 1;
  }

  return 0;
}

static int
writeIdentityRequest (BrailleDisplay *brl) {
  static const unsigned char bytes[] = {NP_REQ_Identify};
  return writeBytes(brl, bytes, sizeof(bytes));
}

static BrailleResponseResult
isIdentityResponse (BrailleDisplay *brl, const void *packet, size_t size) {
  const unsigned char *bytes = packet;

  return (bytes[0] == NP_RSP_Identity)? BRL_RSP_DONE: BRL_RSP_UNEXPECTED;
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  if ((brl->data = malloc(sizeof(*brl->data)))) {
    memset(brl->data, 0, sizeof(*brl->data));
    brl->data->gioEndpoint = NULL;

    if (connectResource(brl, device)) {
      unsigned char response[MAXIMUM_RESPONSE_SIZE];

      if (probeBrailleDisplay(brl, PROBE_RETRY_LIMIT,
                              brl->data->gioEndpoint, PROBE_INPUT_TIMEOUT,
                              writeIdentityRequest,
                              readPacket, &response, sizeof(response),
                              isIdentityResponse)) {
        if (setModel(brl, response[1])) {
          {
            const KeyTableDefinition *ktd = &KEY_TABLE_DEFINITION(all);

            brl->keyBindings = ktd->bindings;
            brl->keyNameTables = ktd->names;
          }

          makeOutputTable(dotsTable_ISO11548_1);
          brl->data->forceRewrite = 1;
          brl->data->acknowledgementPending = 0;
          return 1;
        }
      }

      gioDisconnectResource(brl->data->gioEndpoint);
    }

    free(brl->data);
  } else {
    logMallocError();
  }

  return 0;
}

static void
brl_destruct (BrailleDisplay *brl) {
  if (brl->data) {
    if (brl->data->gioEndpoint) gioDisconnectResource(brl->data->gioEndpoint);
    free(brl->data);
  }
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  if (!brl->data->acknowledgementPending) {
    if (cellsHaveChanged(brl->data->textCells, brl->buffer, brl->textColumns, NULL, NULL, &brl->data->forceRewrite)) {
      unsigned char cells[brl->textColumns];

      translateOutputCells(cells, brl->data->textCells, brl->textColumns);
      if (!writePacket(brl, NP_PKT_REQ_Write, brl->textColumns, cells)) return 0;
      brl->data->acknowledgementPending = 1;
    }
  }

  return 1;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  unsigned char packet[MAXIMUM_RESPONSE_SIZE];
  size_t size;

  while ((size = readPacket(brl, packet, sizeof(packet)))) {
    switch (packet[0]) {
      case NP_PKT_BEGIN: {
        const unsigned char *bytes = &packet[4];
        size_t count = packet[2] - 1;

        switch (packet[3]) {
          case 0X04:
            if (count > 0) {
              unsigned char set;
              unsigned char key = bytes[0];
              int press = !(key & NP_KEY_RELEASE);
              key &= ~NP_KEY_RELEASE;

              if ((key >= NP_KEY_ROUTING_MIN) && (key <= NP_KEY_ROUTING_MAX)) {
                set = NP_SET_RoutingKey;
                key -= NP_KEY_ROUTING_MIN;
              } else {
                set = NP_SET_NavigationKey;
              }

              enqueueKeyEvent(set, key, press);
              continue;
            }
            break;

          case NP_PKT_RSP_Confirmation:
            if (count > 0) {
              switch (bytes[0]) {
                case 0X7D:
                  brl->data->forceRewrite = 1;
                case 0X7E:
                  brl->data->acknowledgementPending = 0;
                  continue;

                default:
                  break;
              }
            }
            break;

          default:
            break;
        }

        break;
      }

      default:
        break;
    }

    logUnexpectedPacket(packet, size);
  }

  return (errno == EAGAIN)? EOF: BRL_CMD_RESTARTBRL;
}
