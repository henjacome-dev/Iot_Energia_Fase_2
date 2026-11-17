/* Etapa 4: medicion + deslastre automatico local.
 * Wokwi: GPIO14 captura y retiene el par V/I en el chip virtual.
 * La escala ADC y SYNC son recursos del modelo, no calibracion del hardware real.
 */
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_timer.h>
#include <atomic>
#include "energia_control.h"

constexpr uint8_t PIN_V = 34, PIN_I = 35, PIN_TEMP = 4, PIN_PIR = 27;
constexpr uint8_t PIN_RELE = 26, PIN_BUZZER = 25, PIN_ALARMA = 33;
constexpr uint8_t PIN_CONTACTO = 32, PIN_SYNC = 14;
OneWire oneWire(PIN_TEMP);
DallasTemperature termometro(&oneWire);
energia::Control control;
energia::EnergiaDemanda metricas;
energia::LineaSerie linea;
energia::Medicion medida, compartida;
portMUX_TYPE muxMedida = portMUX_INITIALIZER_UNLOCKED;
uint32_t secuenciaCompartida = 0, secuenciaLeida = 0, ventanasOmitidas = 0;
bool medidaVista = false, temperaturaVista = false, temperaturaValida = false;
double temperaturaC = 0;
uint32_t ultimaTemperaturaMs = 0, solicitudTemperaturaMs = 0;
uint32_t ultimoReporteMs = 0, ultimoControlMs = 0;
bool conversionPendiente = false, pausaTemperatura = false, sistemaListo = false;
std::atomic<bool> pausaADC{false};
TaskHandle_t tareaADC = nullptr;
esp_timer_handle_t relojADC = nullptr;

// El temporizador solo notifica. Las lecturas ADC se ejecutan en otra tarea.
void avisoADC(void *) {
  if (tareaADC) xTaskNotifyGive(tareaADC);
}

void adquirirADC(void *) {
  energia::VentanaElectrica ventana;
  bool primera = true, nivelSync = false;
  for (;;) {
    uint32_t avisos = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (pausaADC.load(std::memory_order_relaxed)) { primera = true; continue; }
    uint64_t capturaUs = uint64_t(esp_timer_get_time());
    if (primera) {
      ventana.reiniciar(capturaUs >= energia::PERIODO_US ? capturaUs - energia::PERIODO_US : 0);
      primera = false;
    }
    nivelSync = !nivelSync;
    digitalWrite(PIN_SYNC, nivelSync ? HIGH : LOW);
    delayMicroseconds(5); // Permite que el chip retenga los dos valores del mismo instante.
    uint16_t rawV = analogRead(PIN_V), rawI = analogRead(PIN_I);
    energia::Medicion nueva;
    if (ventana.agregar(rawV, rawI, capturaUs, avisos > 1 ? avisos - 1 : 0, nueva)) {
      portENTER_CRITICAL(&muxMedida);
      compartida = nueva;
      ++secuenciaCompartida;
      portEXIT_CRITICAL(&muxMedida);
    }
  }
}

void recogerMedicion() {
  uint32_t leida;
  energia::Medicion nueva;
  portENTER_CRITICAL(&muxMedida);
  leida = secuenciaCompartida;
  if (leida != secuenciaLeida) nueva = compartida;
  portEXIT_CRITICAL(&muxMedida);
  if (leida == secuenciaLeida) return;
  uint32_t diferencia = leida - secuenciaLeida;
  if (diferencia > 1) {
    ventanasOmitidas += diferencia - 1;
    metricas.romperContinuidad();
  }
  secuenciaLeida = leida; medida = nueva; medidaVista = true;
  metricas.agregar(medida);
}

void actualizarTemperatura() {
  if (pausaTemperatura) return;
  uint32_t ahora = millis();
  if (conversionPendiente && uint32_t(ahora - solicitudTemperaturaMs) >= 800) {
    bool terminada = termometro.isConversionComplete();
    temperaturaC = terminada ? termometro.getTempCByIndex(0) : DEVICE_DISCONNECTED_C;
    temperaturaVista = true;
    temperaturaValida = terminada && energia::finito(temperaturaC) &&
                        temperaturaC >= -55 && temperaturaC <= 125;
    if (temperaturaValida) ultimaTemperaturaMs = millis();
    conversionPendiente = false;
  }
  if (!conversionPendiente && uint32_t(millis() - solicitudTemperaturaMs) >= 1000) {
    termometro.requestTemperatures();
    solicitudTemperaturaMs = millis(); conversionPendiente = true;
  }
}

energia::Entradas entradasActuales() {
  energia::Entradas e;
  e.electricaVista = medidaVista; e.electricaValida = medida.valida;
  e.electricaMs = uint32_t(medida.finUs / 1000);
  e.potenciaW = medida.potenciaW;
  e.temperaturaVista = temperaturaVista; e.temperaturaValida = temperaturaValida;
  e.temperaturaMs = ultimaTemperaturaMs; e.temperaturaC = temperaturaC;
  e.contactoCerrado = digitalRead(PIN_CONTACTO) == HIGH;
  e.presencia = digitalRead(PIN_PIR) == HIGH;
  e.ahoraMs = millis(); // Leer despues de copiar los datos evita marcas futuras.
  e.falloSistema = !sistemaListo || uint32_t(e.ahoraMs - ultimoControlMs) > 300;
  return e;
}

void aplicarSalidas() {
  static bool mandoAnterior = false, alarmaAnterior = false;
  bool mando = control.releConectado(), alarma = control.alarma();
  if (mando != mandoAnterior) {
    digitalWrite(PIN_RELE, mando ? HIGH : LOW); mandoAnterior = mando;
  }
  if (alarma != alarmaAnterior) {
    digitalWrite(PIN_ALARMA, alarma ? HIGH : LOW);
    if (alarma) tone(PIN_BUZZER, 2000);
    else { noTone(PIN_BUZZER); digitalWrite(PIN_BUZZER, LOW); }
    alarmaAnterior = alarma;
  }
}

void imprimirCausas(uint8_t causas) {
  if (!causas) { Serial.print("NINGUNA"); return; }
  bool separador = false;
  const uint8_t bits[] = {energia::POTENCIA, energia::TEMPERATURA, energia::SENSOR, energia::CONTACTO};
  const char *nombres[] = {"POTENCIA", "TEMPERATURA", "SENSOR", "CONTACTO"};
  for (unsigned k = 0; k < 4; ++k) if (causas & bits[k]) {
    if (separador) Serial.print("+");
    Serial.print(nombres[k]); separador = true;
  }
}

void informarEstado() {
  energia::Entradas e = entradasActuales();
  Serial.printf("t=%lu ms | %s | RELE=%u NO=%s | ALARMA=%u | PIR=%u | ",
      (unsigned long)e.ahoraMs, energia::nombreEstado(control.estado()),
      control.releConectado(), e.contactoCerrado ? "CERRADO" : "ABIERTO",
      control.alarma(), e.presencia);
  if (control.temperaturaLista(e)) Serial.printf("T=%.2f C | ", temperaturaC);
  else Serial.print("T=NO_VALIDA_O_VENCIDA | ");
  if (medidaVista) {
    Serial.printf("V=%.3f V I=%.5f A P=%.4f W S=%.4f VA FP=%.3f | MED=%s Q=%u fs=%.1f Hz | ",
        medida.vrms, medida.irms, medida.potenciaW, medida.aparenteVA, medida.fp,
        control.electricaLista(e) ? "VALIDA" : "INVALIDA_O_VENCIDA",
        medida.calidad, medida.frecuenciaHz);
  } else Serial.print("MED=ESPERANDO | ");
  Serial.printf("E=%.9f kWh validos=%.2f s | ", metricas.energiaKWh, metricas.segundosValidos);
  if (metricas.completa()) Serial.printf("D10=%.4f W Dmax=%.4f W | ", metricas.demandaW(), metricas.demandaMaxW);
  else Serial.printf("D10=INCOMPLETA(%.2f s) Dmax=%.4f W | ", metricas.coberturaS(), metricas.demandaMaxW);
  Serial.print("CAUSAS="); imprimirCausas(control.causas());
  Serial.printf(" | omitidas=%lu invalidas=%lu\n", (unsigned long)ventanasOmitidas,
      (unsigned long)metricas.ventanasInvalidas);
}

void anunciarCambios() {
  static energia::Estado anterior = energia::Estado::INICIO;
  static uint8_t causasAnteriores = 0;
  if (control.estado() != anterior || control.causas() != causasAnteriores) {
    Serial.printf("EVENTO t=%lu ms %s -> %s | causas=", (unsigned long)millis(), energia::nombreEstado(anterior),
        energia::nombreEstado(control.estado()));
    imprimirCausas(control.causas());
    Serial.printf(" | P=%.4f W T=%.2f C Q=%u\n", medida.potenciaW, temperaturaC, medida.calidad);
    anterior = control.estado(); causasAnteriores = control.causas();
  }
}

void ayuda() {
  Serial.println("ETAPA 4 - MEDICION Y DESLASTRE AUTOMATICO LOCAL");
  Serial.println("REARMAR | PARAR | ESTADO | AYUDA");
  Serial.println("Ensayos de datos vencidos: PAUSA_ADC | SIGUE_ADC | PAUSA_TEMP | SIGUE_TEMP");
  Serial.println("Corte: P>1.50 W O T>55 C. Rearme manual: datos validos, P<1.50 W Y T<50 C.");
  Serial.println("Chip: modo 0=CA; 1..3=DC calibracion; 4=fallo V bajo; 5=fallo I alto.");
  Serial.println("Los mandos manuales RELE/LED/BUZZER de etapa 2 se sustituyen por el control automatico.");
}

void ejecutarOrden(const char *orden) {
  if (!strcmp(orden, "REARMAR")) {
    energia::Rearme r = control.rearmar(entradasActuales());
    aplicarSalidas(); Serial.printf("REARME %s\n", energia::motivoRearme(r));
  } else if (!strcmp(orden, "PARAR")) {
    control.parar(millis()); aplicarSalidas(); Serial.println("PARADA MANUAL: orden de apertura; consultar contacto NO.");
  } else if (!strcmp(orden, "ESTADO")) informarEstado();
  else if (!strcmp(orden, "AYUDA")) ayuda();
  else if (!strcmp(orden, "PAUSA_ADC")) {
    pausaADC.store(true, std::memory_order_relaxed); metricas.romperContinuidad();
    Serial.println("ENSAYO: adquisicion ADC pausada; se supervisa antiguedad.");
  } else if (!strcmp(orden, "SIGUE_ADC")) {
    pausaADC.store(false, std::memory_order_relaxed);
    Serial.println("ENSAYO: ADC reanudado; el bloqueo exige REARMAR.");
  } else if (!strcmp(orden, "PAUSA_TEMP")) {
    pausaTemperatura = true; Serial.println("ENSAYO: temperatura pausada; se supervisa antiguedad.");
  } else if (!strcmp(orden, "SIGUE_TEMP")) {
    pausaTemperatura = false; termometro.requestTemperatures();
    solicitudTemperaturaMs = millis(); conversionPendiente = true;
    Serial.println("ENSAYO: temperatura reanudada; esperar conversion y usar REARMAR.");
  } else Serial.println("Orden desconocida. Escriba AYUDA.");
}

void setup() {
  digitalWrite(PIN_RELE, LOW); pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_ALARMA, LOW); pinMode(PIN_ALARMA, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW); pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_SYNC, LOW); pinMode(PIN_SYNC, OUTPUT);
  pinMode(PIN_PIR, INPUT); pinMode(PIN_CONTACTO, INPUT_PULLDOWN);
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_V, ADC_11db); analogSetPinAttenuation(PIN_I, ADC_11db);
  analogRead(PIN_V); analogRead(PIN_I); // Inicializar el controlador ADC antes de temporizar.
  termometro.begin(); termometro.setResolution(12); termometro.setWaitForConversion(false);
  Serial.printf("DS18B20 detectados=%u\n", termometro.getDeviceCount());
  termometro.requestTemperatures(); solicitudTemperaturaMs = millis(); conversionPendiente = true;
  ayuda();
  BaseType_t tareaOK = xTaskCreatePinnedToCore(adquirirADC, "adc_pares", 4096, nullptr, 5, &tareaADC, 0);
  esp_timer_create_args_t config = {};
  config.callback = avisoADC; config.dispatch_method = ESP_TIMER_TASK;
  config.name = "aviso_2k"; config.skip_unhandled_events = true;
  esp_err_t crear = tareaOK == pdPASS ? esp_timer_create(&config, &relojADC) : ESP_FAIL;
  control.iniciar(millis()); ultimoControlMs = millis();
  sistemaListo = crear == ESP_OK && esp_timer_start_periodic(relojADC, energia::PERIODO_US) == ESP_OK;
  if (!sistemaListo) Serial.println("FALLO: no se pudo iniciar la adquisicion.");
}

void loop() {
  recogerMedicion(); actualizarTemperatura();
  energia::Entradas e = entradasActuales();
  control.actualizar(e); ultimoControlMs = e.ahoraMs;
  aplicarSalidas(); anunciarCambios();
  for (unsigned limite = 0; limite < 32 && Serial.available(); ++limite) {
    energia::LineaSerie::Resultado r = linea.agregar(char(Serial.read()));
    if (r == energia::LineaSerie::LINEA) ejecutarOrden(linea.texto());
    else if (r == energia::LineaSerie::DESBORDADA) Serial.println("Orden descartada: linea demasiado larga.");
  }
  aplicarSalidas(); anunciarCambios();
  if (uint32_t(millis() - ultimoReporteMs) >= 1000) {
    ultimoReporteMs = millis(); informarEstado();
  }
  delay(1);
}
