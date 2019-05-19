// BUS 0 is on the LKAS module (ASCM) side (50/50 this is correct)
// BUS 2 is on the actuator (EPS) side

static int gm_ascm_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {

  uint32_t addr = to_fwd->RIR>>21;

  if (bus_num == 0) {
    //Forward all messages in this direction

    if (addr == 0xf1) {
      // EBCMBrakePedalPosition: The brake sensor sucks and the ASCM gets mad
      // So force the sensor to read 0 if it's "close enough"
      uint8_t pedal = (to_fwd->RDLR >> 8) & 0xFF;
      if (pedal < 10) {
        to_fwd->RDLR &= 0xFF0F; // Force position to 0
        to_fwd->RDHR |= 0x000F; // Checksum at position 0
      }
    }

    return 2;
  }

  if (bus_num == 2) {
    // Drop all steering + brake messages
    //if ((addr == 241) || (addr == 384)  || (addr == 417) || (addr == 715)) {

    // Drop all steering messages
    if ((addr == 384)) {
        return -1;
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

