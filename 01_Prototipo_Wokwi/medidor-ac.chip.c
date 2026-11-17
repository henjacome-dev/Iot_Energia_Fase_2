// Modelo virtual de SALIDAS ACONDICIONADAS: no emula el interior del SCT o ZMPT.
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  double voltage_out;
  double current_out;
  double represented_current_rms;
  bool priority_on;
  bool non_priority_on;
} ac_sample_t;

static double bounded(double x, double lo, double hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

// Función utilizada tanto por el chip real de Wokwi como por la prueba local.
static ac_sample_t ac_sample(double seconds, double volts_rms,
                             double non_priority_ohms, double phase_degrees,
                             bool contact_closed, unsigned mode) {
  const double pi = 3.14159265358979323846;
  volts_rms = bounded(volts_rms, 0.0, 15.0);
  non_priority_ohms = bounded(non_priority_ohms, 110.0, 440.0);
  phase_degrees = bounded(phase_degrees, 0.0, 90.0);
  double irms = volts_rms / 220.0;
  if (contact_closed) irms += volts_rms / non_priority_ohms;
  const double angle = 2.0 * pi * 60.0 * seconds;
  ac_sample_t value = {
    .voltage_out = 1.65 + 0.05 * sqrt(2.0) * volts_rms * sin(angle),
    .current_out = 1.65 + 2.0 * sqrt(2.0) * irms * sin(angle - phase_degrees * pi / 180.0),
    .represented_current_rms = irms,
    .priority_on = volts_rms > 0.000001,
    .non_priority_on = contact_closed && volts_rms > 0.000001,
  };
  // 0=señales CA; 1,2,3=puntos DC para caracterizar el ADC del simulador.
  if (mode >= 1 && mode <= 3) {
    const double levels[] = {0.0, 1.0, 1.65, 2.5};
    value.voltage_out = levels[mode];
    value.current_out = levels[mode];
  }
  if (mode == 4) value.voltage_out = 0.0; // Falla de ensayo: saturacion baja V.
  if (mode == 5) value.current_out = 5.0; // Falla de ensayo: saturacion alta I.
  return value;
}

#ifndef MODELO_AC_NATIVE_TEST
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t vout, iout, contact, sync, priority_led, non_priority_led;
  uint32_t voltage_attr, resistance_attr, phase_attr, mode_attr;
  double voltage, resistance, phase;
  unsigned mode, ticks;
  int previous_contact, previous_priority, previous_non_priority;
} chip_state_t;

static void capture_pair(chip_state_t *chip) {
  ac_sample_t sample = ac_sample(get_sim_nanos() / 1000000000.0,
      chip->voltage, chip->resistance, chip->phase,
      pin_read(chip->contact) == HIGH, chip->mode);
  // Retencion comun: ambos canales corresponden al MISMO instante del modelo.
  pin_dac_write(chip->vout, (float)sample.voltage_out);
  pin_dac_write(chip->iout, (float)sample.current_out);
}

static void sync_changed(void *user_data, pin_t pin, uint32_t value) {
  (void)pin; (void)value;
  capture_pair((chip_state_t *)user_data);
}

static void update_signals(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;
  // Atributos cada 10 ms, supervision de ramas cada 100 us. V/I se retienen en SYNC.
  if (chip->ticks++ % 100 == 0) {
    chip->voltage = attr_read_float(chip->voltage_attr);
    chip->resistance = attr_read_float(chip->resistance_attr);
    chip->phase = attr_read_float(chip->phase_attr);
    chip->mode = attr_read(chip->mode_attr);
  }
  bool closed = pin_read(chip->contact) == HIGH;
  bool priority = chip->voltage > 0.000001, non_priority = priority && closed;
  if (chip->previous_priority != (int)priority) {
    pin_write(chip->priority_led, priority ? HIGH : LOW);
    chip->previous_priority = priority;
  }
  if (chip->previous_non_priority != (int)non_priority) {
    pin_write(chip->non_priority_led, non_priority ? HIGH : LOW);
    chip->previous_non_priority = non_priority;
  }
  if (chip->previous_contact != (int)closed) {
    printf("[MODELO] Contacto NO %s; I_referencia=%.6f A RMS\n",
           closed ? "CERRADO" : "ABIERTO",
           chip->voltage / 220.0 + (closed ? chip->voltage / chip->resistance : 0.0));
    chip->previous_contact = closed;
  }
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)calloc(1, sizeof(chip_state_t));
  chip->vout = pin_init("VOUT", ANALOG);
  chip->iout = pin_init("IOUT", ANALOG);
  chip->contact = pin_init("CONTACT", INPUT_PULLDOWN);
  chip->sync = pin_init("SYNC", INPUT_PULLDOWN);
  chip->priority_led = pin_init("PRIO", OUTPUT_LOW);
  chip->non_priority_led = pin_init("NOPRIO", OUTPUT_LOW);
  chip->voltage_attr = attr_init_float("voltageRms", 12.0f);
  chip->resistance_attr = attr_init_float("nonPriorityOhms", 220.0f);
  chip->phase_attr = attr_init_float("phaseDegrees", 0.0f);
  chip->mode_attr = attr_init("mode", 0);
  chip->previous_contact = -1;
  chip->previous_priority = chip->previous_non_priority = -1;
  pin_watch_config_t watch = {.edge = BOTH, .pin_change = sync_changed, .user_data = chip};
  pin_watch(chip->sync, &watch);
  timer_config_t config = {.callback = update_signals, .user_data = chip};
  timer_t timer = timer_init(&config);
  update_signals(chip);
  capture_pair(chip);
  timer_start(timer, 100, true);
}
#endif
