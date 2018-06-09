// sample struct that keeps 3 samples in memory
struct sample_t {
  int values[6];
  int min;
  int max;
} sample_t_default = {{0}, 0, 0};

void safety_rx_hook(CAN_FIFOMailBox_TypeDef *to_push);
int safety_tx_hook(CAN_FIFOMailBox_TypeDef *to_send);
int safety_tx_lin_hook(int lin_num, uint8_t *data, int len);
int safety_ignition_hook();
uint32_t get_ts_elapsed(uint32_t ts, uint32_t ts_last);
int to_signed(int d, int bits);
void update_sample(struct sample_t *sample, int sample_new);

typedef void (*safety_hook_init)(int16_t param);
typedef void (*rx_hook)(CAN_FIFOMailBox_TypeDef *to_push);
typedef int (*tx_hook)(CAN_FIFOMailBox_TypeDef *to_send);
typedef int (*tx_lin_hook)(int lin_num, uint8_t *data, int len);
typedef int (*ign_hook)();
typedef int (*fwd_hook)(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd);

typedef struct {
  safety_hook_init init;
  ign_hook ignition;
  rx_hook rx;
  tx_hook tx;
  tx_lin_hook tx_lin;
  fwd_hook fwd;
} safety_hooks;

// This can be set by the safety hooks.
int controls_allowed = 0;

// Include the actual safety policies.
#include "safety/safety_defaults.h"
#include "safety/safety_honda.h"
#include "safety/safety_toyota.h"
#ifdef PANDA
#include "safety/safety_toyota_ipas.h"
#include "safety/safety_gm_ascm.h"
#endif
#include "safety/safety_gm.h"
#include "safety/safety_ford.h"
#include "safety/safety_cadillac.h"
#include "safety/safety_elm327.h"

const safety_hooks *current_hooks = &nooutput_hooks;

void safety_rx_hook(CAN_FIFOMailBox_TypeDef *to_push){
  current_hooks->rx(to_push);
}

int safety_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  return current_hooks->tx(to_send);
}

int safety_tx_lin_hook(int lin_num, uint8_t *data, int len){
  return current_hooks->tx_lin(lin_num, data, len);
}

// -1 = Disabled (Use GPIO to determine ignition)
// 0 = Off (not started)
// 1 = On (started)
int safety_ignition_hook() {
  return current_hooks->ignition();
}
int safety_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  return current_hooks->fwd(bus_num, to_fwd);
}

typedef struct {
  uint16_t id;
  const safety_hooks *hooks;
} safety_hook_config;

#define SAFETY_NOOUTPUT 0
#define SAFETY_HONDA 1
#define SAFETY_TOYOTA 2
#define SAFETY_GM 3
#define SAFETY_HONDA_BOSCH 4
#define SAFETY_FORD 5
#define SAFETY_CADILLAC 6
#define SAFETY_GM_ASCM 0x1334
#define SAFETY_TOYOTA_IPAS 0x1335
#define SAFETY_TOYOTA_NOLIMITS 0x1336
#define SAFETY_ALLOUTPUT 0x1337
#define SAFETY_ELM327 0xE327

const safety_hook_config safety_hook_registry[] = {
  {SAFETY_NOOUTPUT, &nooutput_hooks},
  {SAFETY_HONDA, &honda_hooks},
  {SAFETY_HONDA_BOSCH, &honda_bosch_hooks},
  {SAFETY_TOYOTA, &toyota_hooks},
  {SAFETY_GM, &gm_hooks},
  {SAFETY_FORD, &ford_hooks},
  {SAFETY_CADILLAC, &cadillac_hooks},
  {SAFETY_TOYOTA_NOLIMITS, &toyota_nolimits_hooks},
#ifdef PANDA
  {SAFETY_TOYOTA_IPAS, &toyota_ipas_hooks},
  {SAFETY_GM_ASCM, &gm_ascm_hooks},
#endif
  {SAFETY_ALLOUTPUT, &alloutput_hooks},
  {SAFETY_ELM327, &elm327_hooks},
};

#define HOOK_CONFIG_COUNT (sizeof(safety_hook_registry)/sizeof(safety_hook_config))

int safety_set_mode(uint16_t mode, int16_t param) {
  for (int i = 0; i < HOOK_CONFIG_COUNT; i++) {
    if (safety_hook_registry[i].id == mode) {
      current_hooks = safety_hook_registry[i].hooks;
      if (current_hooks->init) current_hooks->init(param);
      return 0;
    }
  }
  return -1;
}

// compute the time elapsed (in microseconds) from 2 counter samples
uint32_t get_ts_elapsed(uint32_t ts, uint32_t ts_last) {
  return ts > ts_last ? ts - ts_last : (0xFFFFFFFF - ts_last) + 1 + ts;
}

// convert a trimmed integer to signed 32 bit int
int to_signed(int d, int bits) {
  if (d >= (1 << (bits - 1))) {
    d -= (1 << bits);
  }
  return d;
}

// given a new sample, update the smaple_t struct
void update_sample(struct sample_t *sample, int sample_new) {
  for (int i = sizeof(sample->values)/sizeof(sample->values[0]) - 1; i > 0; i--) {
    sample->values[i] = sample->values[i-1];
  }
  sample->values[0] = sample_new;

  // get the minimum and maximum measured samples
  sample->min = sample->max = sample->values[0];
  for (int i = 1; i < sizeof(sample->values)/sizeof(sample->values[0]); i++) {
    if (sample->values[i] < sample->min) sample->min = sample->values[i];
    if (sample->values[i] > sample->max) sample->max = sample->values[i];
  }
}
