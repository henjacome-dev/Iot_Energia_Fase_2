# Verificación de la etapa 4

Fecha: 28 de octubre de 2026, Guayaquil. Se distinguen las pruebas locales del núcleo y la ejecución del ESP32 en Wokwi. El paquete integra las mediciones necesarias para el deslastre; no agrega componentes de compra.

## Pruebas automáticas locales

Se ejecutó `python3 verificacion/ejecutar.py`. El modelo C y el núcleo C++ compilaron con optimización y comprobaciones `-Wall -Wextra -Werror -pedantic`. Resultado del núcleo: **37 casos correctos, 0 fallos**, más las comprobaciones del modelo C. La salida íntegra se conserva en `salida_pruebas.txt`.

Los casos comprueban arranque, corte por cada causa y por ambas simultáneamente, bloqueo retenido, rearme por debajo de los límites, rechazo de igualdades en el rearme, valores NaN e infinitos, rangos inválidos, vencimientos exactos, contacto incoherente, PIR, desbordamiento de `millis()`, órdenes serie incompletas o demasiado largas, medición eléctrica, energía, demanda y huecos de datos. Incluyen un lazo cerrado que utiliza la función matemática del chip y el mismo núcleo de medición y control que incluye el firmware.

Referencias calculadas por ese núcleo con cuantización de ensayo de 12 bits:

| Caso | V RMS | I RMS | P activa | S aparente | FP |
| --- | ---: | ---: | ---: | ---: | ---: |
| Normal, ambas ramas de 220 Ω | 12,000069 V | 0,109103 A | 1,309246 W | 1,309248 VA | 0,999998 |
| Sobrecarga, rama desconectable de 110 Ω | 12,000069 V | 0,163613 A | 1,963364 W | 1,963366 VA | 0,999999 |
| Solo prioritaria | 12,000069 V | 0,054556 A | 0,654677 W | 0,654681 VA | 0,999994 |
| Señales con desfase de 60° | 12,000069 V | 0,109100 A | 0,654573 W | 1,309204 VA | 0,499978 |

Se comprobaron las tolerancias de la ficha: ±máximo(1 %, 0,05 V), ±máximo(1 %, 0,002 A) y ±máximo(1 %, 0,01 W). La integración de 60 s a potencia normal se contrastó con 0,000021818182 kWh; la demanda no se declaró completa antes de diez segundos. Un hueco temporal entre ventanas válidas reinicia la demanda y no se integra como energía conocida.

Estas pruebas ejecutan el núcleo real, pero no emulan el planificador, el ADC, las bibliotecas o el cableado de Wokwi.

## Ejecución funcional en Wokwi

El proyecto completo compiló y ejecutó en el navegador. Se usaron los seis archivos de ejecución, DS18B20 y PIR nativos, relé con continuidad COM/NO, alarma y chip de señales con captura común SYNC. El ESP32 virtual utiliza `cpuFrequency: "16"`.

| Caso observado | Evidencia del monitor | Resultado |
| --- | --- | --- |
| Arranque | INICIO con NO abierto, seguido de `INICIO -> ESPERA`, sin causa retenida. | La rama desconectable no se conecta sola. |
| Medición solo prioritaria | A t=2000 ms: V=11,998 V, I=0,05456 A, P=0,6546 W, Q=0 y fs=2001,1 Hz. | Concordancia con la referencia. |
| Habilitación manual | `REARME ACEPTADO`, seguido de `ESPERA -> NORMAL`. | Relé cerrado y alarma apagada. |
| Medición normal estable | A t=3000–10000 ms: P≈1,3085 W, I≈0,10906 A, Q=0; contadores de ventanas inválidas y omitidas en cero. | Concordancia con 1,309091 W y 0,109091 A. |
| Sobrecarga | Se ajustó la rama desconectable a 110 Ω; aparece `NORMAL -> BLOQUEADO` por POTENCIA. A t=11000 ms: NO abierto, ALARMA=1 y P=0,6546 W. | Deslastre automático con conservación de la rama prioritaria. |
| Retención del bloqueo | A t=12000 y 13000 ms la potencia ya era baja, pero permanecía BLOQUEADO por POTENCIA. | No se reconecta por normalizar la medición. |
| Recuperación de la carga | Tras restablecer 220 Ω y enviar `REARMAR`, aparece `BLOQUEADO -> NORMAL`. | Rearme válido comprobado. |
| Sobretemperatura | Se ajustó el DS18B20 a 60 °C; aparece bloqueo por TEMPERATURA. A t=15000 ms: T=60,00 °C, NO abierto y ALARMA=1. | Corte térmico comprobado. |
| Límite de rearme | Con T=50,00 °C se registra `REARME RECHAZADO: T debe ser menor que 50 C`. | La igualdad de 50 °C se rechaza. |
| Normalización térmica | Con T=49,00 °C, a t=25000–26000 ms seguía BLOQUEADO; después de `REARMAR`, a t=27000 ms estaba NORMAL, NO cerrado y ALARMA=0. | Rearme manual bajo el límite comprobado. |
| Adquisición detenida | Tras `PAUSA_ADC`, a t=30000 ms: BLOQUEADO por SENSOR, NO abierto y `MED=INVALIDA_O_VENCIDA`; la energía queda en 0,000006884 kWh durante los reportes siguientes. | Corte por dato eléctrico vencido y ausencia de energía inventada durante el hueco. |
| Rearme con datos vencidos | Durante la pausa se registra `REARME RECHAZADO: datos invalidos, vencidos o sistema no listo`. | No se permite reconectar sin una medición actualizada. |
| Demanda completa | A t=11000 ms se muestra D10=1,1576 W y Dmax=1,1576 W para la trayectoria de cargas ensayada. | Se muestra una ventana completa; ese valor no corresponde a una carga constante. |

El archivo `wokwi_sesion_control.txt` conserva el monitor de esta sesión. Los tiempos de la tabla son del ESP32 simulado. La potencia alta que causó un disparo puede existir solo durante una ventana; el resumen periódico posterior ya muestra la potencia reducida de la rama prioritaria.

Durante la pausa, V, I y P conservan el último valor adquirido y aparecen marcados como vencidos; no describen el consumo posterior a la apertura. Q=0 describe la calidad de esa última ventana, mientras que `MED=INVALIDA_O_VENCIDA` indica su falta de vigencia. La compilación de cierre y el arranque del paquete final quedan registrados por separado en `wokwi_version_final.txt`; esa versión incluye tiempo, potencia y temperatura en cada evento y comprobación de huecos temporales en la demanda.

La versión final recompilada pasó de INICIO a ESPERA a t=1230 ms. Después del rearme, a t=7000–9000 ms mostró NORMAL, NO cerrado, alarma apagada, T=49,00 °C, P=1,3086 W y Q=0, sin ventanas inválidas ni omitidas. La temperatura de 49 °C se conservó en el componente al reiniciar la sesión de prueba; el archivo de circuito distribuido conserva 25 °C como valor inicial para un proyecto nuevo.

Durante la preparación, el límite de ejecución predeterminado del simulador produjo pérdidas de avisos, correctamente señaladas por Q=4 y bloqueo SENSOR. Se ajustó el límite virtual a 16 MHz y se repitió la sesión funcional: las ventanas observadas quedaron válidas. Se mantuvieron las comprobaciones de calidad; no se ocultaron las pérdidas.

## Alcance y pruebas de la siguiente etapa

Los umbrales exactos de potencia y la igualdad de 55 °C se verificaron con entradas numéricas del núcleo. Las perillas analógicas tienen resolución y cuantización, por lo que no sustituyen esas pruebas de igualdad. El caso de desfase de 60° y la energía de 60 s de esta entrega son comprobaciones locales del núcleo; su contraste prolongado con el firmware completo, la repetición sistemática de fallos de cada canal, el vencimiento térmico, las causas simultáneas y la captura de frecuencia del buzzer se reunirán en la matriz de la etapa 5.

La sesión de Wokwi no es una validación física de los sensores ni de la precisión del ESP32 real. La interfaz de relé, el acondicionamiento y la calibración físicos conservan el alcance de la fase posterior.
