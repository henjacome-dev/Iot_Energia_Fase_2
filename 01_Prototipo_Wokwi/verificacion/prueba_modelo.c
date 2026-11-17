// Compilar: gcc -std=c11 -Wall -Wextra -Werror prueba_modelo.c -lm -o prueba_modelo
#define MODELO_AC_NATIVE_TEST
#include "../medidor-ac.chip.c"
#include <assert.h>
#include <stdio.h>

static void check_case(const char *name, double r, bool contact,
                       double phase, double expected_i, double expected_p) {
  double vv=0, ii=0, vi=0;
  for (unsigned k=0; k<200; ++k) {
    ac_sample_t a=ac_sample(k/2000.0,12.0,r,phase,contact,0);
    double v=(a.voltage_out-1.65)/0.05;
    double i=(a.current_out-1.65)/2.0;
    vv+=v*v; ii+=i*i; vi+=v*i;
    assert(a.voltage_out>=0 && a.voltage_out<=3.3);
    assert(a.current_out>=0 && a.current_out<=3.3);
    assert(a.priority_on);
    assert(a.non_priority_on==contact);
  }
  assert(fabs(sqrt(vv/200)-12.0)<1e-10);
  assert(fabs(sqrt(ii/200)-expected_i)<1e-10);
  assert(fabs(vi/200-expected_p)<1e-10);
  printf("OK %-25s I=%.9f A P=%.9f W\n",name,sqrt(ii/200),vi/200);
}

int main(void) {
  check_case("normal cerrado",220,true,0,24.0/220,288.0/220);
  check_case("sobrecarga cerrado",110,true,0,12.0/220+12.0/110,144.0/220+144.0/110);
  check_case("deslastre por contacto",110,false,0,12.0/220,144.0/220);
  check_case("desfase 60 grados",220,true,60,24.0/220,144.0/220);
  const double dc[]={0,1.0,1.65,2.5};
  for(unsigned m=1;m<=3;++m){
    ac_sample_t a=ac_sample(0.01,12,220,0,true,m);
    assert(a.voltage_out==dc[m] && a.current_out==dc[m]);
  }
  for(unsigned k=0;k<200;++k){
    ac_sample_t a=ac_sample(k/2000.0,15,110,90,true,0);
    assert(a.voltage_out>=0.589 && a.voltage_out<=2.712);
    assert(a.current_out>=0 && a.current_out<=3.3);
  }
  ac_sample_t off=ac_sample(0.01,0,110,0,true,0);
  assert(!off.priority_on && !off.non_priority_on);
  puts("OK puntos DC, limites analogicos y alimentacion representada cero");
  return 0;
}
