#include "f_pro.h"

#define UNUSED(x) (void)(x)

static uint8_t reply_8101[] = {
	0x81, 0x01, 0x00, 0x03, 0x2b, 0x3c, 0x42, 0xd6, 0x03, 0x04
};

static uint8_t reply_8102[] = {
	0x81, 0x02
};

static uint8_t reply_8103[] = {
	0x81, 0x03
};

#define EXTRA_DATA_START (12)
uint8_t input_reply_21[0x40] = {
	// 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15
	0x21, 0x00, 0x91, 0x00, 0x80, 0x00, 0x19, 0x68, 0x73, 0x3e, 0xa8, 0x73, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t reply_02_data[] = {
	0x00, 0x82, 0x02, 0x03, 0x48, 0x03, 0x02, 0x04, 0x03, 0xD6, 0x42, 0x3c, 0x2b, 0x03, 0x01
};

static const uint8_t reply_03_data[] = {
  0x00, 0x80, 0x03
};

static const uint8_t reply_40_data[] = {
  0x00, 0x80, 0x40
};

static const uint8_t reply_41_data[] = {
  0x00, 0x80, 0x41
};

static const uint8_t reply_04_data[] = {
	0x00, 0x83, 0x04
};

static const uint8_t reply_30_data[] = {
  0x00, 0x80, 0x30
};

static const uint8_t reply_38_data[] = {
  0x00, 0x80, 0x38
};

static const uint8_t reply_08_data[] = {
	0x00, 0x80, 0x08
};

static uint8_t reply_10_data[] = {
	0x00, 0x90, 0x10, 0xcc, 0xcc, 0xcc, 0xcc, 0xdd,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};

const uint8_t reply_01_04_data[] = {
	0x00, 0x81, 0x01, 0x03
};

static const uint8_t reply_48_data[] = {
  0x00, 0x80, 0x48
};

static const u8 spi_flash_6000[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const u8 spi_flash_6050[] = {
  0x32, 0x32, 0x32,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static int handle_spi_flash_read(u32 address, u8 len, u8 *data) {
  if (address == 0x6000) {
    memcpy(data, spi_flash_6000, len);
  } else if (address == 0x6050) {
    memcpy(data, spi_flash_6050, len);
  }
  return 0;
}

static int handle_subcommand_input(u8 subcommand, const u8 *sub_data, u16 sub_len, struct f_switchpro *sp) {
  u8 buf[0x40];
  const u8 *reply = NULL;
  int reply_len = 0;
  int result = 0;
  int val;

#ifdef DEBUG
  printk("pro_reply: subcommand: %02x [%02x, %02x, %02x, %02x]",
    subcommand, sub_data[0], sub_data[1], sub_data[2], sub_data[3]);
#endif

  switch (subcommand) {
    case 0x03: {
      reply = reply_03_data;
      reply_len = sizeof(reply_03_data);
      result = RBEGIN_REPORT_STANDARD;
      break;
    }
    case 0x02: {
      reply = reply_02_data;
      reply_len = sizeof(reply_02_data);
      break;
    }
    case 0x08: {
      reply = reply_08_data;
      reply_len = sizeof(reply_08_data);
      break;
    }
    // SPI flash read
    case 0x10: {
      u32 address = *(u32*)(sub_data);
      u8 len = sub_data[4];

      // printk("pro_reply: %02x %02x %02x %02x %02x %02x", sub_data[0], sub_data[1], sub_data[2], sub_data[3], sub_data[4], sub_data[5], sub_data[6]);
      printk("pro_reply: SPI flash read: %08x %d", address, len);
      handle_spi_flash_read(address, len, reply_10_data + 8);
      memcpy(reply_10_data + 3, sub_data, 4 + 1); // address, len
      reply = reply_10_data;
      reply_len = sizeof(reply_10_data);
      break;
    }
    // enable vibration
    case 0x48: {
      reply = reply_48_data;
      reply_len = sizeof(reply_48_data);
      break;
    }
    case 0x01: {
      reply = reply_01_04_data;
      reply_len = sizeof(reply_01_04_data);
      break;
    }
    case 0x40: {
      reply = reply_40_data;
      reply_len = sizeof(reply_40_data);
      break;
    }
    case 0x41: {
      reply = reply_41_data;
      reply_len = sizeof(reply_41_data);
      break;
    }
    case 0x30: {
      reply = reply_30_data;
      reply_len = sizeof(reply_30_data);
      break;
    }
    case 0x38: {
      reply = reply_38_data;
      reply_len = sizeof(reply_38_data);
      break;
    }
    case 0x04: {
      reply = reply_04_data;
      reply_len = sizeof(reply_04_data);
      break;
    }
    default: {
      // const uint8_t default_reply_data[] = {
      //   0x00, 0x80, 0x03
      // };
      // reply = default_reply_data;
      // reply_len = sizeof(default_reply_data);
    }
  }

  // ACK
  if (reply) {
    memcpy(buf, input_reply_21, 0x40);
    memcpy(buf + EXTRA_DATA_START, reply, reply_len);
    val = send_report(sp, buf, sizeof(buf));
    if (val) {
      return val;
    }
  }
  return result;
}

static int handle_pro_input(const u8 *input, u16 input_len, struct f_switchpro *sp) {
  if (input[0] == 0x80) {
    if (input[1] == 0x01) {
      return send_report(sp, reply_8101, sizeof(reply_8101));
    } else if (input[1] == 0x02) {
      return send_report(sp, reply_8102, sizeof(reply_8102));
    } else if (input[1] == 0x03) {
      return send_report(sp, reply_8103, sizeof(reply_8103));
    } else if (input[1] == 0x04) {
      return 0; // just ignore it. force pro to talk over USB
    }
  } else if (input[0] == 0x01) {
    // rumble and subcommand
    u8 counter = input[1];
    // u8 rumble_data[8];
    // memcpy(rumble_data, input + 2, 8);
    u8 subcommand = input[10];
    const u8 *sub_data = input + 11;
    int sub_len = input_len - 11;

    UNUSED(counter);

    return handle_subcommand_input(subcommand, sub_data, sub_len, sp);
  } else if (input[0] == 0x10) {
    // rumble only
    return 0; // skip it
  }

  return -EOPNOTSUPP;
}
