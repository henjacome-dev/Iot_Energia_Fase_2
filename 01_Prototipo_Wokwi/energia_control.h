#pragma once
// Nucleo comun al ESP32 y a las pruebas locales. Sin dependencias de Arduino.
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

namespace energia {
constexpr double LIMITE_W = 1.50;
constexpr double CORTE_C = 55.0;
constexpr double REARME_C = 50.0;
constexpr uint32_t MAX_EDAD_E_MS = 300, MAX_EDAD_T_MS = 3000;
constexpr uint32_t ASENTAMIENTO_RELE_MS = 100;
constexpr unsigned PARES_POR_VENTANA = 200;
constexpr uint32_t PERIODO_US = 500;
constexpr double ADC_PASO_V = 5.0 / 4096.0; // Solo Wokwi, caracterizado en etapa 2.

inline bool finito(double x) { return isfinite(x); }
inline double mayor(double a, double b) { return a > b ? a : b; }

enum Calidad : uint16_t {
  CAL_OK = 0, ADC_SATURADO = 1, OFFSET_INVALIDO = 2,
  MUESTREO_INVALIDO = 4, RANGO_INVALIDO = 8
};

struct Medicion {
  bool valida = false;
  uint16_t calidad = CAL_OK, pares = 0;
  uint64_t finUs = 0;
  double duracionS = 0, frecuenciaHz = 0;
  double vrms = 0, irms = 0, potenciaW = 0, aparenteVA = 0, fp = 0;
  double offsetV = 0, offsetI = 0;
  uint32_t intervaloMaxUs = 0, avisosPerdidos = 0;
};

class VentanaElectrica {
  unsigned n_ = 0;
  double sv_ = 0, si_ = 0, svv_ = 0, sii_ = 0, svi_ = 0;
  uint64_t inicio_ = 0, primero_ = 0, ultimo_ = 0;
  uint16_t calidad_ = 0;
  uint32_t intervaloMax_ = 0, perdidos_ = 0;
 public:
  void reiniciar(uint64_t inicioUs) {
    n_ = 0; sv_ = si_ = svv_ = sii_ = svi_ = 0;
    inicio_ = inicioUs; primero_ = ultimo_ = 0;
    calidad_ = 0; intervaloMax_ = perdidos_ = 0;
  }
  bool agregar(uint16_t rawV, uint16_t rawI, uint64_t instanteUs,
               uint32_t avisosPerdidos, Medicion &salida) {
    if (rawV <= 8 || rawV >= 4087 || rawI <= 8 || rawI >= 4087)
      calidad_ |= ADC_SATURADO;
    if (n_) {
      if (instanteUs <= ultimo_) calidad_ |= MUESTREO_INVALIDO;
      else {
        uint64_t dt = instanteUs - ultimo_;
        if (dt > intervaloMax_) intervaloMax_ = uint32_t(dt);
        if (dt > 1500) calidad_ |= MUESTREO_INVALIDO;
      }
    } else primero_ = instanteUs;
    ultimo_ = instanteUs;
    perdidos_ += avisosPerdidos;
    double v = rawV * ADC_PASO_V, i = rawI * ADC_PASO_V;
    sv_ += v; si_ += i; svv_ += v * v; sii_ += i * i; svi_ += v * i;
    if (++n_ < PARES_POR_VENTANA) return false;
    salida = Medicion{};
    salida.finUs = instanteUs;
    salida.pares = uint16_t(n_);
    salida.duracionS = instanteUs > inicio_ ? (instanteUs - inicio_) / 1e6 : 0;
    salida.frecuenciaHz = instanteUs > primero_ ?
        (n_ - 1) * 1e6 / (instanteUs - primero_) : 0;
    salida.offsetV = sv_ / n_; salida.offsetI = si_ / n_;
    salida.vrms = sqrt(mayor(0, svv_ / n_ - salida.offsetV * salida.offsetV)) / 0.05;
    salida.irms = sqrt(mayor(0, sii_ / n_ - salida.offsetI * salida.offsetI)) / 2.00;
    salida.potenciaW = (svi_ / n_ - salida.offsetV * salida.offsetI) / (0.05 * 2.00);
    salida.aparenteVA = salida.vrms * salida.irms;
    salida.fp = salida.aparenteVA > 1e-9 ? salida.potenciaW / salida.aparenteVA : 0;
    if (salida.fp > 1) salida.fp = 1;
    if (salida.fp < -1) salida.fp = -1;
    if (fabs(salida.offsetV - 1.65) > 0.10 || fabs(salida.offsetI - 1.65) > 0.10)
      calidad_ |= OFFSET_INVALIDO;
    if (perdidos_ || salida.frecuenciaHz < 1800 || salida.frecuenciaHz > 2200 ||
        salida.duracionS < 0.09 || salida.duracionS > 0.11)
      calidad_ |= MUESTREO_INVALIDO;
    // Margen del 1% en el extremo superior por cuantizacion del modelo.
    if (!finito(salida.potenciaW) || !finito(salida.vrms) || !finito(salida.irms) ||
        salida.vrms > 15.15 || salida.irms > 0.303)
      calidad_ |= RANGO_INVALIDO;
    salida.calidad = calidad_;
    salida.valida = calidad_ == CAL_OK;
    salida.intervaloMaxUs = intervaloMax_;
    salida.avisosPerdidos = perdidos_;
    reiniciar(instanteUs);
    return true;
  }
};

class EnergiaDemanda {
  struct Segmento { double p = 0, s = 0; } datos_[128];
  unsigned primero_ = 0, cantidad_ = 0;
  double segundos_ = 0, juliosVentana_ = 0;
  uint64_t ultimoFinUs_ = 0;
 public:
  double energiaKWh = 0, segundosValidos = 0, demandaMaxW = 0;
  uint32_t ventanasInvalidas = 0;
  void romperContinuidad() {
    primero_ = cantidad_ = 0; segundos_ = juliosVentana_ = 0; ultimoFinUs_ = 0;
  }
  void agregar(const Medicion &m) {
    if (!m.valida || !finito(m.duracionS) || m.duracionS <= 0 || m.duracionS > 0.11) {
      ++ventanasInvalidas; romperContinuidad(); return;
    }
    if (m.finUs && ultimoFinUs_ && (m.finUs <= ultimoFinUs_ ||
        double(m.finUs - ultimoFinUs_) > m.duracionS * 1e6 + 1.0)) romperContinuidad();
    ultimoFinUs_ = m.finUs;
    energiaKWh += m.potenciaW * m.duracionS / 3600000.0;
    segundosValidos += m.duracionS;
    if (cantidad_ == 128) romperContinuidad();
    unsigned pos = (primero_ + cantidad_++) % 128;
    datos_[pos].p = m.potenciaW; datos_[pos].s = m.duracionS;
    segundos_ += m.duracionS; juliosVentana_ += m.potenciaW * m.duracionS;
    while (cantidad_ && segundos_ > 10.0 + 1e-9) {
      Segmento &viejo = datos_[primero_];
      double retirar = segundos_ - 10.0;
      if (retirar > viejo.s) retirar = viejo.s;
      segundos_ -= retirar; juliosVentana_ -= viejo.p * retirar; viejo.s -= retirar;
      if (viejo.s < 1e-9) { primero_ = (primero_ + 1) % 128; --cantidad_; }
    }
    if (completa() && demandaW() > demandaMaxW) demandaMaxW = demandaW();
  }
  bool completa() const { return segundos_ >= 10.0 - 1e-6; }
  double coberturaS() const { return segundos_; }
  double demandaW() const { return segundos_ > 0 ? juliosVentana_ / segundos_ : 0; }
};

enum class Estado : uint8_t { INICIO, ESPERA, NORMAL, BLOQUEADO };
enum Causa : uint8_t { NINGUNA = 0, POTENCIA = 1, TEMPERATURA = 2, SENSOR = 4, CONTACTO = 8 };
struct Entradas {
  uint32_t ahoraMs = 0;
  bool electricaVista = false, electricaValida = false;
  uint32_t electricaMs = 0;
  double potenciaW = 0;
  bool temperaturaVista = false, temperaturaValida = false;
  uint32_t temperaturaMs = 0;
  double temperaturaC = 0;
  bool contactoCerrado = false, falloSistema = false, presencia = false;
};
enum class Rearme : uint8_t { ACEPTADO, YA_NORMAL, DATOS, POTENCIA_ALTA, TEMPERATURA_ALTA, CONTACTO_CERRADO };

class Control {
  Estado estado_ = Estado::INICIO;
  uint8_t retenidas_ = 0, activas_ = 0;
  bool rele_ = false;
  uint32_t inicioMs_ = 0, cambioReleMs_ = 0;
  void fijarMando(bool conectar, uint32_t ahora) {
    if (rele_ != conectar) { rele_ = conectar; cambioReleMs_ = ahora; }
  }
 public:
  void iniciar(uint32_t ahora) {
    estado_ = Estado::INICIO; retenidas_ = activas_ = 0; rele_ = false;
    inicioMs_ = cambioReleMs_ = ahora;
  }
  bool electricaLista(const Entradas &e) const {
    return e.electricaVista && e.electricaValida && finito(e.potenciaW) &&
           uint32_t(e.ahoraMs - e.electricaMs) <= MAX_EDAD_E_MS;
  }
  bool temperaturaLista(const Entradas &e) const {
    return e.temperaturaVista && e.temperaturaValida && finito(e.temperaturaC) &&
           e.temperaturaC >= -55 && e.temperaturaC <= 125 &&
           uint32_t(e.ahoraMs - e.temperaturaMs) <= MAX_EDAD_T_MS;
  }
  void actualizar(const Entradas &e) {
    const bool el = electricaLista(e), te = temperaturaLista(e);
    const bool falloE = e.electricaVista ? !el : uint32_t(e.ahoraMs - inicioMs_) > MAX_EDAD_E_MS;
    const bool falloT = e.temperaturaVista ? !te : uint32_t(e.ahoraMs - inicioMs_) > MAX_EDAD_T_MS;
    activas_ = 0;
    if (falloE || falloT || e.falloSistema) activas_ |= SENSOR;
    if (el && e.potenciaW > LIMITE_W) activas_ |= POTENCIA;
    if (te && e.temperaturaC > CORTE_C) activas_ |= TEMPERATURA;
    if (activas_) { retenidas_ |= activas_; estado_ = Estado::BLOQUEADO; }
    else if (estado_ == Estado::INICIO && el && te) estado_ = Estado::ESPERA;
    fijarMando(estado_ == Estado::NORMAL, e.ahoraMs);
    if (e.contactoCerrado != rele_ && uint32_t(e.ahoraMs - cambioReleMs_) > ASENTAMIENTO_RELE_MS) {
      activas_ |= CONTACTO; retenidas_ |= CONTACTO; estado_ = Estado::BLOQUEADO;
      fijarMando(false, e.ahoraMs);
    }
  }
  Rearme rearmar(const Entradas &e) {
    actualizar(e);
    if (!electricaLista(e) || !temperaturaLista(e) || e.falloSistema) return Rearme::DATOS;
    if (e.potenciaW >= LIMITE_W) return Rearme::POTENCIA_ALTA;
    if (e.temperaturaC >= REARME_C) return Rearme::TEMPERATURA_ALTA;
    if (estado_ == Estado::NORMAL) return Rearme::YA_NORMAL;
    if (e.contactoCerrado) return Rearme::CONTACTO_CERRADO;
    retenidas_ = activas_ = 0; estado_ = Estado::NORMAL;
    fijarMando(true, e.ahoraMs);
    return Rearme::ACEPTADO;
  }
  void parar(uint32_t ahora) {
    if (estado_ == Estado::NORMAL) estado_ = Estado::ESPERA;
    fijarMando(false, ahora); // PARAR no borra un bloqueo ni silencia su alarma.
  }
  Estado estado() const { return estado_; }
  bool releConectado() const { return rele_; }
  bool alarma() const { return estado_ == Estado::BLOQUEADO; }
  uint8_t causas() const { return retenidas_; }
  uint8_t causasActivas() const { return activas_; }
};

inline const char *nombreEstado(Estado e) {
  switch (e) {
    case Estado::INICIO: return "INICIO";
    case Estado::ESPERA: return "ESPERA";
    case Estado::NORMAL: return "NORMAL";
    default: return "BLOQUEADO";
  }
}
inline const char *motivoRearme(Rearme r) {
  switch (r) {
    case Rearme::ACEPTADO: return "ACEPTADO";
    case Rearme::YA_NORMAL: return "YA ESTABA NORMAL";
    case Rearme::DATOS: return "RECHAZADO: datos invalidos, vencidos o sistema no listo";
    case Rearme::POTENCIA_ALTA: return "RECHAZADO: P debe ser menor que 1.50 W";
    case Rearme::TEMPERATURA_ALTA: return "RECHAZADO: T debe ser menor que 50 C";
    default: return "RECHAZADO: el contacto debe estar abierto antes del rearme";
  }
}

// Descarta una linea completa si excede su capacidad; nunca interpreta la cola.
class LineaSerie {
  char texto_[64] = {};
  size_t usados_ = 0;
  bool descartando_ = false;
 public:
  enum Resultado { NADA, LINEA, DESBORDADA };
  Resultado agregar(char c) {
    if (c == '\r') return NADA;
    if (c == '\n') {
      if (descartando_) { usados_ = 0; descartando_ = false; texto_[0] = 0; return DESBORDADA; }
      texto_[usados_] = 0;
      size_t primero = 0;
      while (primero < usados_ && isspace((unsigned char)texto_[primero])) ++primero;
      while (usados_ > primero && isspace((unsigned char)texto_[usados_ - 1])) --usados_;
      size_t longitud = usados_ - primero;
      memmove(texto_, texto_ + primero, longitud); texto_[longitud] = 0;
      for (size_t k = 0; k < longitud; ++k) texto_[k] = char(toupper((unsigned char)texto_[k]));
      usados_ = 0; return longitud ? LINEA : NADA;
    }
    if (descartando_) return NADA;
    if (usados_ + 1 >= sizeof(texto_)) { descartando_ = true; return NADA; }
    texto_[usados_++] = c; return NADA;
  }
  const char *texto() const { return texto_; }
};
} // namespace energia
