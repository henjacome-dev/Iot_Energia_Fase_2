# Especificación de la versión entregada

Fecha: 29 de octubre de 2026. Fase del curso: segunda, programación e integración local simulada. Base de ejecución: etapa 4, validada por el equipo en etapa 5.

## Componentes y representación

El controlador es un ESP32 DevKit V1 clásico, programado en C++ con Arduino y ejecutado en Wokwi. Las cuatro entradas corresponden a corriente, tensión, temperatura y presencia. El SCT013-000 y ZMPT101B se representan mediante un chip de señales acondicionadas; DS18B20 y PIR utilizan componentes virtuales de Wokwi. Los actuadores son el relé de la rama no prioritaria y la alarma de LED rojo y buzzer. Los LED verde y azul muestran el estado de las ramas representadas.

La alimentación de carga representada es monofásica de 12 V RMS y 60 Hz. La rama prioritaria equivale a 220 Ω. La rama no prioritaria vale 220 Ω en funcionamiento normal y 110 Ω para la sobrecarga. Los 12 VCA y estas resistencias se calculan dentro del modelo. COM/NO utiliza un lazo lógico de 3,3 V para supervisar el contacto; no se conecta una red industrial al ESP32.

## Parámetros vigentes

| Parámetro | Valor o regla |
| --- | --- |
| Valores iniciales | 12 V RMS, 220 Ω no prioritarios, 0° de desfase, modo 0 y DS18B20 a 25 °C. |
| Atributo del ESP32 | `cpuFrequency: "16"` en `diagram.json`; conservarlo para el muestreo validado. |
| Límite de potencia | Corte si P > 1,50 W. La igualdad no dispara por sí sola. |
| Límite térmico | Corte si T > 55 °C. La igualdad no dispara por sí sola. |
| Rearme | Orden `REARMAR`, datos válidos y recientes, P < 1,50 W, T < 50 °C y contacto abierto. |
| Alarma | LED rojo y tono de 2 kHz durante BLOQUEADO, hasta un rearme válido. |
| Ventana eléctrica | 200 pares de muestras; objetivo 2.000 pares/s y ventana nominal de 100 ms. |
| Demanda | Media móvil de 10 s válidos consecutivos; máximo de ventanas completas durante la sesión. |
| Energía | Integra únicamente intervalos válidos medidos; comienza en cero al reiniciar. |
| Antigüedad máxima | Electricidad: 300 ms. Temperatura: 3.000 ms. Un dato más antiguo bloquea. |
| Supervisión del contacto | Discrepancia entre mando y NO durante más de 100 ms genera CONTACTO. |
| Monitor | 115.200 bit/s, resumen cada segundo y eventos al cambiar estado o causas. |
| Presencia | Informativa. No anula los bloqueos ni ordena cortar la rama prioritaria. |

La demanda de 10 s sirve para la demostración. El corte usa la potencia activa de cada ventana eléctrica y no espera a completar esos diez segundos. El intervalo y la tarifa de una instalación real requieren una configuración específica.

## Conexiones

| Señal | GPIO | Correspondencia |
| --- | ---: | --- |
| Tensión | 34 | `medidor:VOUT`, ADC1. |
| Corriente | 35 | `medidor:IOUT`, ADC1. |
| Temperatura | 4 | DQ del DS18B20, pull-up de 4,7 kΩ a 3,3 V. |
| Presencia | 27 | OUT del PIR. |
| Mando de relé | 26 | HIGH conecta y LOW abre. |
| LED de alarma | 33 | Resistencia de 330 Ω y LED rojo. |
| Buzzer | 25 | Tono de alarma. |
| Realimentación del contacto | 32 | NO, compartido con `medidor:CONTACT`, pull-down de 10 kΩ. |
| Captura común virtual | 14 | `medidor:SYNC`. |

GPIO14 y GPIO32 son incorporaciones de la versión integrada respecto de la ficha inicial. SYNC captura V e I en el mismo instante del modelo y permite leerlos secuencialmente sin introducir un desfase ficticio. Es un recurso de la simulación, no una prestación de muestreo simultáneo del ADC físico.

## Magnitudes y calidad

El firmware corrige el offset de cada canal y calcula V e I RMS, potencia activa, potencia aparente y factor de potencia. La energía utiliza la potencia activa y la duración de las ventanas válidas. Una interrupción de datos reinicia la cobertura de demanda; conserva el máximo histórico y no estima energía para rellenar el hueco.

Las salidas virtuales se relacionan con las señales representadas mediante `VOUT = 1,65 + 0,05 v(t)` e `IOUT = 1,65 + 2 i(t)`. La escala de ensayo ADC es `5/4096 V por cuenta`, caracterizada para este simulador.

Una ventana deja de ser válida si hay saturación, offset inadecuado, rango inválido, avisos perdidos o calidad temporal insuficiente. En el monitor, Q=0 corresponde a una ventana de calidad aceptable; `MED=VALIDA` también exige que la medición siga vigente. Durante una pausa pueden mostrarse V/I/P anteriores: consultar siempre MED.

## Estados y órdenes

| Estado | Relé no prioritario | Alarma | Salida normal |
| --- | --- | --- | --- |
| INICIO | Abierto | Apagada durante inicialización | Obtener datos válidos. Un fallo o timeout bloquea. |
| ESPERA | Abierto | Apagada | `REARMAR` válido permite NORMAL. |
| NORMAL | Cerrado | Apagada | Supervisar continuamente. |
| BLOQUEADO | Abierto | Encendida | Retener causas y esperar rearme manual válido. |

Las causas POTENCIA, TEMPERATURA, SENSOR y CONTACTO pueden acumularse. Normalizar una entrada no reconecta la carga. `PARAR` abre el relé; desde NORMAL pasa a ESPERA y, durante un bloqueo, conserva el bloqueo y su alarma.

Órdenes disponibles: `REARMAR`, `PARAR`, `ESTADO`, `AYUDA`, `PAUSA_ADC`, `SIGUE_ADC`, `PAUSA_TEMP` y `SIGUE_TEMP`. Las pausas son recursos de ensayo de antigüedad. Los mandos directos `RELE 1`, `LED 0` y `BUZZER 0` de versiones iniciales se rechazan.

## Referencias para la demostración

| Condición a 12 V RMS | I RMS esperada | P activa esperada |
| --- | ---: | ---: |
| Ambas ramas de 220 Ω conectadas | 0,109091 A | 1,309091 W |
| Sobrecarga de 110 Ω, antes del corte | 0,163636 A | 1,963636 W |
| Solo la rama prioritaria | 0,054545 A | 0,654545 W |

El valor de sobrecarga puede aparecer únicamente en el evento que causa el corte. El resumen posterior ya representa la rama prioritaria. Restablecer 220 Ω antes de un rearme estable: si se deja la sobrecarga, el sistema puede aceptar el rearme con potencia baja y volver a disparar al conectarla.

La ficha fija tolerancias de V ±máximo(1 %, 0,05 V), I ±máximo(1 %, 0,002 A), P ±máximo(1 %, 0,01 W) y energía 2 % en un intervalo estable de al menos diez segundos. La matriz contiene los registros y criterios para comprobarlas.

## Alcance

Se entrega una simulación funcional con control local. La calibración física, el acondicionamiento real, la capacidad de maniobra de un montaje industrial y la conectividad MQTT con TLS y dashboard corresponden a la fase de hardware. El caso comercial propone históricos persistentes como mejora; esa propuesta no modifica el firmware validado.
