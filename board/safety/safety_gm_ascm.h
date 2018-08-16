// BUS 0 is on the LKAS module (ASCM) side
// BUS 2 is on the actuator (EPS) side

static int gm_ascm_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {

  uint32_t addr = to_fwd->RIR>>21;

  if (bus_num == 0) {

    // do not propagate lkas messages from ascm to actuators
    // block 0x180 from ASCM for steering torque control
    if (addr == 0x180) {
      int lkas_on = 0;
      if (!lkas_on) return -1;
    }

    return 2;
  }

  if (bus_num == 2) {
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
};

