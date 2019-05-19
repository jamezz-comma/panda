// BUS 0 is on the LKAS module (ASCM) side (50/50 this is correct)
// BUS 2 is on the actuator (EPS) side

static int gm_ascm_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {

  uint32_t addr = to_fwd->RIR>>21;

  if (bus_num == 0) {
    //Forward all messages in this direction
    return 2;
  }

  if (bus_num == 2) {
    // Drop all steering + brake messages
    //if ((addr == 241) || (addr == 384)  || (addr == 417) || (addr == 715)) {

    // Drop all steering messages
    if ((addr == 384)) {
        return -1;
    }

    if (addr == 0xf1) {
      // EBCMBrakePedalPosition
      uint8_t pedal = (to_fwd->RDLR >> 16) & 0xFF;
      if (pedal < 10) {
        to_fwd->RDLR &= 0xF0FF; // Force position to 0
        to_fwd->RDHR |= 0xFF00; // Checksum at position 0
      }
    }

    return 0;
  }

  return -1;
}

const safety_hooks gm_ascm_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = alloutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .ignition = default_ign_hook,
  .fwd = gm_ascm_fwd_hook,
  .relay = nooutput_relay_hook,
};

