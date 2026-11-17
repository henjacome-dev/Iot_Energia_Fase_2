#include "../energia_control.h"
#define MODELO_AC_NATIVE_TEST
#include "../medidor-ac.chip.c"
#include <iostream>
#include <iomanip>
#include <limits>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

using namespace energia;
static void exigir(bool ok, const char *detalle) { if (!ok) throw std::runtime_error(detalle); }
static void cerca(double real, double referencia, double tolerancia, const char *detalle) {
  if (!finito(real) || fabs(real - referencia) > tolerancia) throw std::runtime_error(detalle);
}
static Entradas frescas(uint32_t ahora = 1000, double p = 0.6545454545,
                       double t = 25, bool contacto = false) {
  Entradas e;
  e.ahoraMs = e.electricaMs = e.temperaturaMs = ahora;
  e.electricaVista = e.electricaValida = true;
  e.temperaturaVista = e.temperaturaValida = true;
  e.potenciaW = p; e.temperaturaC = t; e.contactoCerrado = contacto;
  return e;
}
static Control normal() {
  Control c; c.iniciar(0);
  exigir(c.rearmar(frescas()) == Rearme::ACEPTADO, "preparar estado NORMAL");
  return c;
}
static uint16_t cuantizar(double u) {
  double q = floor(double(float(u)) * 4096.0 / 5.0);
  if (q < 0) q = 0;
  if (q > 4095) q = 4095;
  return uint16_t(q);
}
static Medicion medir(double v = 12, double r = 220, double fase = 0,
                     bool contacto = true, unsigned modo = 0, uint32_t perdido = 0) {
  VentanaElectrica w; w.reiniciar(0); Medicion m;
  bool lista = false;
  for (unsigned k = 1; k <= 200; ++k) {
    uint64_t us = k * 500;
    ac_sample_t s = ac_sample(us / 1e6, v, r, fase, contacto, modo);
    lista = w.agregar(cuantizar(s.voltage_out), cuantizar(s.current_out), us,
                     k == 100 ? perdido : 0, m);
  }
  exigir(lista, "ventana de 200 pares incompleta");
  return m;
}

int main() {
  std::vector<std::pair<std::string, std::function<void()>>> casos;
  auto caso = [&](const char *nombre, std::function<void()> funcion) { casos.emplace_back(nombre, funcion); };
  caso("arranque abierto y espera sin conexion automatica", [] {
    Control c; c.iniciar(0);
    exigir(!c.releConectado() && !c.alarma(), "salidas de inicio");
    c.actualizar(frescas());
    exigir(c.estado() == Estado::ESPERA && !c.releConectado(), "espera abierta");
  });
  caso("rearme valido conecta y apaga alarma", [] {
    Control c = normal(); exigir(c.releConectado() && !c.alarma(), "rearme no conecta");
  });
  caso("potencia exactamente 1.50 W no dispara", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.50, 25, true));
    exigir(c.estado() == Estado::NORMAL, "igualdad de potencia");
  });
  caso("potencia inmediatamente superior dispara", [] {
    Control c = normal(); c.actualizar(frescas(1100, std::nextafter(1.50, 2.0), 25, true));
    exigir(c.alarma() && !c.releConectado() && c.causas() == POTENCIA, "corte por potencia");
  });
  caso("temperatura exactamente 55 C no dispara", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.3, 55, true));
    exigir(c.estado() == Estado::NORMAL, "igualdad termica");
  });
  caso("temperatura 55.0625 C dispara", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.3, 55.0625, true));
    exigir(c.alarma() && !c.releConectado() && c.causas() == TEMPERATURA, "corte termico");
  });
  caso("causas simultaneas quedan retenidas", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.96, 60, true));
    exigir(c.causas() == (POTENCIA | TEMPERATURA), "faltan causas simultaneas");
  });
  caso("normalizar y PARAR no borran bloqueo", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.96, 25, true));
    c.actualizar(frescas(1300)); c.parar(1400);
    exigir(c.estado() == Estado::BLOQUEADO && c.alarma() && c.causas() == POTENCIA,
            "reconexion o borrado automatico");
    exigir(c.rearmar(frescas(1500)) == Rearme::ACEPTADO && !c.causas(), "borrar tras rearme valido");
  });
  caso("rearme rechazado a 50 C y aceptado debajo", [] {
    Control c = normal(); c.actualizar(frescas(1100, 1.3, 60, true));
    exigir(c.rearmar(frescas(1400, .65, 50)) == Rearme::TEMPERATURA_ALTA, "rearme a 50 C");
    exigir(!c.releConectado(), "rearme rechazado cerro rele");
    exigir(c.rearmar(frescas(1500, .65, 49.9375)) == Rearme::ACEPTADO, "rearme bajo 50 C");
  });
  caso("rearme rechazado a 1.50 W y aceptado debajo", [] {
    Control c; c.iniciar(0);
    exigir(c.rearmar(frescas(1000, 1.50)) == Rearme::POTENCIA_ALTA, "rearme a limite potencia");
    exigir(c.rearmar(frescas(1001, 1.499)) == Rearme::ACEPTADO, "rearme potencia menor");
  });
  caso("datos invalidos impiden reconectar", [] {
    Control c = normal(); Entradas e = frescas(1100, 1.3, 25, true); e.electricaValida = false;
    c.actualizar(e); e.contactoCerrado = false;
    exigir(c.causas() & SENSOR, "falta causa sensor");
    exigir(c.rearmar(e) == Rearme::DATOS && !c.releConectado(), "rearme con dato invalido");
  });
  caso("NaN e infinito se consideran fallo", [] {
    for (double v : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
      Control c = normal(); c.actualizar(frescas(1100, v, 25, true));
      exigir(c.alarma() && (c.causas() & SENSOR), "potencia no finita");
      Control d = normal(); d.actualizar(frescas(1100, 1.3, v, true));
      exigir(d.alarma() && (d.causas() & SENSOR), "temperatura no finita");
    }
  });
  caso("temperatura desconectada o fuera de rango bloquea", [] {
    for (double t : {-127.0, -55.1, 125.1}) {
      Control c = normal(); c.actualizar(frescas(1100, 1.3, t, true));
      exigir(c.alarma() && (c.causas() & SENSOR), "rango termico");
    }
  });
  caso("electrica valida hasta 300 ms y vencida a 301 ms", [] {
    Control c = normal(); Entradas e = frescas(1400, 1.3, 25, true); e.electricaMs = 1100;
    c.actualizar(e); exigir(c.releConectado(), "caduca antes de 300 ms");
    e.ahoraMs = 1401; c.actualizar(e);
    exigir(c.alarma() && (c.causas() & SENSOR), "no caduca a 301 ms");
  });
  caso("temperatura valida hasta 3000 ms y vencida a 3001 ms", [] {
    Control c = normal(); Entradas e = frescas(4000, 1.3, 25, true); e.temperaturaMs = 1000;
    c.actualizar(e); exigir(c.releConectado(), "caduca antes de 3000 ms");
    e.ahoraMs = 4001; c.actualizar(e);
    exigir(c.alarma() && (c.causas() & SENSOR), "no caduca a 3001 ms");
  });
  caso("inicio sin electrica vence tras 300 ms", [] {
    Control c; c.iniciar(0); Entradas e; e.ahoraMs = 300;
    c.actualizar(e); exigir(!c.alarma(), "gracia electrica recortada");
    e.ahoraMs = 301; c.actualizar(e); exigir(c.alarma(), "inicio sin electrica no bloquea");
  });
  caso("inicio sin temperatura vence tras 3000 ms", [] {
    Control c; c.iniciar(0); Entradas e = frescas(3000); e.temperaturaVista = false;
    c.actualizar(e); exigir(!c.alarma(), "gracia termica recortada");
    e = frescas(3001); e.temperaturaVista = false; c.actualizar(e);
    exigir(c.alarma(), "inicio sin temperatura no bloquea");
  });
  caso("contacto que no cierra genera fallo tras asentamiento", [] {
    Control c = normal(); c.actualizar(frescas(1100)); exigir(c.releConectado(), "gracia del rele");
    c.actualizar(frescas(1101)); exigir(c.alarma() && (c.causas() & CONTACTO), "contacto no supervisado");
  });
  caso("contacto cerrado pese a orden de apertura impide rearme", [] {
    Control c; c.iniciar(0); Entradas e = frescas(1000, .65, 25, true);
    c.actualizar(e); exigir(c.causas() & CONTACTO, "contacto pegado no detectado");
    exigir(c.rearmar(e) == Rearme::CONTACTO_CERRADO && !c.releConectado(), "rearme con contacto pegado");
  });
  caso("PIR no cambia prioridad ni proteccion", [] {
    Control c = normal(); Entradas e = frescas(1100, 1.3, 25, true);
    e.presencia = true; c.actualizar(e); exigir(c.releConectado(), "PIR corta carga");
    e.presencia = false; e.potenciaW = 2; c.actualizar(e); exigir(c.alarma(), "PIR anula proteccion");
  });
  caso("reloj millis soporta desbordamiento uint32", [] {
    uint32_t inicio = UINT32_MAX - 1000; Control c; c.iniciar(inicio);
    Entradas e = frescas(UINT32_MAX - 100);
    exigir(c.rearmar(e) == Rearme::ACEPTADO, "rearme antes de wrap");
    e.ahoraMs = 50; e.contactoCerrado = true; c.actualizar(e);
    exigir(c.releConectado(), "edad correcta despues de wrap");
    e.ahoraMs = 201; c.actualizar(e); exigir(c.alarma(), "edad vencida despues de wrap");
  });
  caso("rearme prematuro y fallo de sistema rechazados", [] {
    Control c; c.iniciar(0); Entradas e; e.ahoraMs = 100;
    exigir(c.rearmar(e) == Rearme::DATOS, "rearme sin datos");
    e = frescas(); e.falloSistema = true;
    exigir(c.rearmar(e) == Rearme::DATOS && c.alarma(), "rearme con fallo del sistema");
  });
  caso("PARAR desde NORMAL abre y deja ESPERA", [] {
    Control c = normal(); c.parar(1100);
    exigir(c.estado() == Estado::ESPERA && !c.releConectado() && !c.alarma(), "parada manual");
  });
  caso("orden normalizada, CRLF y linea vacia", [] {
    LineaSerie l; LineaSerie::Resultado r = LineaSerie::NADA;
    for (char c : std::string("  rearmar \r\n")) r = l.agregar(c);
    exigir(r == LineaSerie::LINEA && !strcmp(l.texto(), "REARMAR"), "normalizar orden");
    exigir(l.agregar('\n') == LineaSerie::NADA, "linea vacia");
  });
  caso("orden demasiado larga descarta tambien su cola", [] {
    LineaSerie l; LineaSerie::Resultado r = LineaSerie::NADA;
    for (char c : std::string(90, 'X') + "REARMAR\n") r = l.agregar(c);
    exigir(r == LineaSerie::DESBORDADA, "cola de orden larga aceptada");
    for (char c : std::string("ESTADO\n")) r = l.agregar(c);
    exigir(r == LineaSerie::LINEA && !strcmp(l.texto(), "ESTADO"), "recuperacion parser");
  });
  caso("medicion normal con cuantizacion del modelo", [] {
    Medicion m = medir(); exigir(m.valida, "normal invalida");
    cerca(m.vrms, 12, .12, "VRMS"); cerca(m.irms, 12.0/110, .002, "IRMS");
    cerca(m.potenciaW, 144.0/110, .014, "potencia activa"); cerca(m.fp, 1, .001, "FP unitario");
  });
  caso("medicion sobrecarga y apertura conserva rama prioritaria", [] {
    Medicion a = medir(12, 110), b = medir(12, 110, 0, false);
    exigir(a.valida && b.valida && a.potenciaW > LIMITE_W && b.potenciaW < LIMITE_W, "cambio de carga");
    cerca(a.potenciaW, 1.96363636, .02, "potencia sobrecarga");
    cerca(b.potenciaW, .65454545, .01, "potencia tras corte");
  });
  caso("desfase 60 grados distingue potencia activa y aparente", [] {
    Medicion m = medir(12, 220, 60); exigir(m.valida, "desfase invalido");
    cerca(m.aparenteVA, 1.3090909, .014, "S con desfase");
    cerca(m.potenciaW, .65454545, .01, "P con desfase"); cerca(m.fp, .5, .01, "FP 0.5");
  });
  caso("cero voltios representados no implica sensor averiado", [] {
    Medicion m = medir(0); exigir(m.valida, "cero no debe ser fallo");
    cerca(m.vrms, 0, 1e-4, "cero VRMS"); cerca(m.potenciaW, 0, 1e-7, "cero potencia");
  });
  caso("saturaciones analogicas y offset incorrecto invalidan", [] {
    exigir(medir(12,220,0,true,4).calidad & ADC_SATURADO, "modo fallo V");
    exigir(medir(12,220,0,true,5).calidad & ADC_SATURADO, "modo fallo I");
    exigir(medir(12,220,0,true,1).calidad & OFFSET_INVALIDO, "offset 1V");
    exigir(medir(12,220,0,true,3).calidad & OFFSET_INVALIDO, "offset 2.5V");
  });
  caso("avisos de muestreo perdidos invalidan ventana", [] {
    Medicion m = medir(12,220,0,true,0,1);
    exigir(!m.valida && (m.calidad & MUESTREO_INVALIDO), "perdida de muestras ignorada");
  });
  caso("muestreo irregular excesivo se detecta", [] {
    VentanaElectrica w; w.reiniciar(0); Medicion m;
    for (uint64_t k = 1; k <= 200; ++k) w.agregar(1351,1351,k*2000,0,m);
    exigir(!m.valida && (m.calidad & MUESTREO_INVALIDO), "frecuencia fuera de objetivo");
  });
  caso("energia de 60 s y demanda completa de 10 s", [] {
    EnergiaDemanda d; Medicion m; m.valida = true; m.potenciaW = 144.0/110; m.duracionS = .1;
    for (int k = 0; k < 99; ++k) d.agregar(m);
    exigir(!d.completa(), "demanda completa antes de 10 s"); d.agregar(m);
    exigir(d.completa(), "demanda incompleta a 10 s");
    for (int k = 100; k < 600; ++k) d.agregar(m);
    cerca(d.energiaKWh, .0000218181818181818, 1e-12, "integracion energia");
    cerca(d.demandaW(), m.potenciaW, 1e-10, "demanda constante");
    cerca(d.demandaMaxW, m.potenciaW, 1e-10, "maximo demanda");
  });
  caso("demanda movil ponderada retira segmento antiguo", [] {
    EnergiaDemanda d; Medicion m; m.valida=true; m.duracionS=.1; m.potenciaW=1;
    for (int k=0;k<100;++k) d.agregar(m);
    m.potenciaW=2; for (int k=0;k<10;++k) d.agregar(m);
    cerca(d.demandaW(),1.1,1e-10,"promedio movil 9s a 1W mas 1s a 2W");
    cerca(d.coberturaS(),10,1e-10,"ventana exactamente diez segundos");
  });
  caso("ventana invalida no inventa energia y reinicia demanda", [] {
    EnergiaDemanda d; Medicion m; m.valida=true; m.duracionS=.1; m.potenciaW=1;
    for (int k=0;k<100;++k) d.agregar(m);
    double antes=d.energiaKWh; m.valida=false; d.agregar(m);
    exigir(!d.completa() && d.ventanasInvalidas==1,"demanda tras hueco");
    cerca(d.energiaKWh,antes,1e-15,"energia durante hueco");
    cerca(d.demandaMaxW,1,1e-10,"maximo historico conservado");
  });
  caso("hueco entre ventanas validas reinicia demanda sin estimar energia", [] {
    EnergiaDemanda d; Medicion m; m.valida=true; m.duracionS=.1; m.potenciaW=1;
    for (uint64_t k=1;k<=100;++k) { m.finUs=k*100000; d.agregar(m); }
    exigir(d.completa(),"demanda previa al hueco");
    double antes=d.energiaKWh; m.finUs=12000000; d.agregar(m);
    exigir(!d.completa(),"hueco temporal no detectado");
    cerca(d.coberturaS(),.1,1e-12,"cobertura despues de hueco");
    cerca(d.energiaKWh,antes+.1/3600000,1e-15,"no integrar intervalo perdido");
  });
  caso("lazo modelo-medicion-control corta sobrecarga y calor", [] {
    Control c; c.iniciar(0); VentanaElectrica w; w.reiniciar(0); Medicion m;
    bool vista=false, corteP=false, corteT=false, ramaConservada=false, retuvo=false;
    for (uint64_t us=500;us<=3000000;us+=500) {
      uint32_t ms=uint32_t(us/1000); double r=ms>=1500 && ms<2200 ? 110:220;
      double t=ms>=2500 && ms<2700 ? 60:25;
      ac_sample_t s=ac_sample(us/1e6,12,r,0,c.releConectado(),0);
      if (w.agregar(cuantizar(s.voltage_out),cuantizar(s.current_out),us,0,m)) vista=true;
      Entradas e=frescas(ms,vista?m.potenciaW:0,t,c.releConectado());
      e.electricaVista=vista; e.electricaValida=m.valida; e.electricaMs=uint32_t(m.finUs/1000);
      c.actualizar(e);
      if (us==1000000 || us==2300000 || us==2800000)
        exigir(c.rearmar(e)==Rearme::ACEPTADO,"rearme en lazo");
      if (ms>=1600 && ms<2200 && (c.causas()&POTENCIA) && !c.releConectado()) corteP=true;
      if (ms>=1800 && ms<2200 && s.priority_on && !s.non_priority_on && m.potenciaW<.67) ramaConservada=true;
      if (ms>=2500 && ms<2700 && (c.causas()&TEMPERATURA) && !c.releConectado()) corteT=true;
      if (ms==2750 && c.estado()==Estado::BLOQUEADO && !c.releConectado()) retuvo=true;
    }
    exigir(corteP && ramaConservada && corteT && retuvo,"resultado del lazo integrado");
  });

  int fallos=0;
  for (const auto &c : casos) {
    try { c.second(); std::cout << "OK " << c.first << '\n'; }
    catch (const std::exception &e) { ++fallos; std::cout << "FALLO " << c.first << ": " << e.what() << '\n'; }
  }
  std::cout << std::fixed << std::setprecision(6);
  for (const auto &casoR : std::vector<std::pair<const char *,Medicion>>{
      {"normal",medir()},{"sobrecarga",medir(12,110)},
      {"solo_prioritaria",medir(12,110,0,false)},{"desfase_60",medir(12,220,60)}}) {
    const Medicion &m=casoR.second;
    std::cout << "REFERENCIA " << casoR.first << " V=" << m.vrms << " I=" << m.irms
              << " P=" << m.potenciaW << " S=" << m.aparenteVA << " FP=" << m.fp << '\n';
  }
  std::cout << "TOTAL " << casos.size() << " | CORRECTAS " << casos.size()-fallos << " | FALLOS " << fallos << '\n';
  return fallos ? 1:0;
}
