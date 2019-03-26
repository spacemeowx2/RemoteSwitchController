#include "f_pro.h"

static uint8_t reply_8101[] = {
	0x81, 0x01, 0x00, 0x03, 0x6c, 0x27, 0x1b, 0xd6, 0x03, 0x04
};

static uint8_t reply_8102[] = {
	0x81, 0x02
};

static uint8_t reply_8103[] = {
	0x81, 0x03
};

static uint8_t reply_02_data[] = {
	0x00, 0x82, 0x02, 0x03, 0x48, 0x03, 0x02, 0x04, 0x03, 0xD6, 0x1B, 0x27, 0x6C, 0x03, 0x01
};

uint8_t input_reply_21[0x40] = {
	0x21, 0x00, 0x91, 0x00, 0x80, 0x00, 0x19, 0x68, 0x73, 0x3e, 0xa8, 0x73, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int handle_subcommand_input(u8 subcommand, const u8 *sub_data, u16 sub_len, struct f_switchpro *sp) {
  u8 buf[0x40];
  u8 *reply = { 0x00, 0x82, 0x80, subcommand, 0x03 };
  int reply_len = 3;

  // ACK
  memcpy(buf, input_reply_21, 0x40);
  if (reply) {
    memcpy(buf + 12, reply, reply_len);
  }
  return send_report(sp, buf, sizeof(buf));
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
    return handle_subcommand_input(subcommand, sub_data, sub_len, sp);
  }

  return -EOPNOTSUPP;
}
