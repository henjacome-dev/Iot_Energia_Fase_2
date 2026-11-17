# Ficha técnica de la etapa 1 del proyecto IoT energético

Fecha: 28 de octubre de 2026. Estado: especificación definida para construir la simulación en la etapa 2. Se verificaron las referencias técnicas y los cálculos de referencia; la ejecución en Wokwi y las pruebas del firmware corresponden a las etapas siguientes.

El sistema representará una instalación monofásica de 12 VCA con una carga prioritaria y otra desconectable. El ESP32 medirá tensión, corriente, temperatura y presencia, calculará potencia y energía, y abrirá el relé de la carga no prioritaria por exceso de potencia o temperatura. El monitor serie permitirá observar el comportamiento y ordenar el rearme manual.

**Parámetros de trabajo definidos.** Salvo el corte térmico de 55 °C exigido por el enunciado, los valores siguientes son decisiones de diseño de nuestra maqueta.

| Parámetro | Valor o condición |
| --- | --- |
| Controlador y lenguaje | ESP32 DevKit V1, familia ESP32 clásica, programado en C++ con el entorno Arduino. |
| Simulador | Wokwi, con modelos de señales eléctricas y componentes virtuales DS18B20, PIR, relé, buzzer y LED. |
| Tensión y frecuencia nominales representadas | 12,00 V RMS, 60 Hz. |
| Rango de ensayo eléctrico | 0 a 15 V RMS y 0 a 0,30 A RMS. Son rangos del modelo, no las capacidades nominales del SCT013-000. |
| Carga prioritaria | Equivalente resistivo de 220 Ω, permanentemente conectado mientras exista alimentación. |
| Carga no prioritaria normal | Equivalente resistivo de 220 Ω, controlado por el relé. |
| Sobrecarga de prueba | Cambiar virtualmente la rama no prioritaria a 110 Ω. Este valor pertenece al modelo de prueba. |
| Límite de potencia activa | 1,50 W. Representa el umbral de la demostración, sin atribuirle un significado tarifario real. |
| Corte por temperatura | T > 55 °C. |
| Rearme | Orden manual, sensores válidos, P < 1,50 W y T < 50 °C. |
| Muestreo eléctrico objetivo | 2.000 pares tensión-corriente por segundo, con tiempos registrados y alineación temporal. |
| Ventana de cálculo de potencia y RMS | 100 ms: 200 pares y seis ciclos de 60 Hz. |
| Demanda de demostración | Media móvil de potencia de 10 segundos; indicar ventana incompleta durante los primeros 10 segundos. |
| Demanda máxima | Mayor media móvil completa de 10 segundos registrada durante la sesión. |
| Temperatura | Lectura objetivo cada segundo, resolución de 12 bits y conversión sin bloquear el muestreo eléctrico. |
| Datos envejecidos | Falla si la medición eléctrica no se actualiza durante más de 300 ms o la temperatura válida durante más de 3 segundos. |
| Monitor serie | 115.200 bit/s, resumen cada segundo y mensajes inmediatos en cambios de estado. |
| Alarma | LED rojo encendido y buzzer de 2 kHz durante un bloqueo por disparo o fallo, hasta rearme válido. |
| Reinicio de métricas | Energía y demanda máxima comienzan en cero al reiniciar la simulación. |

El corte utilizará la potencia activa calculada en ventanas de 100 ms. La demanda media de 10 segundos será otro indicador: no se esperará a completar esa ventana para desconectar una carga. Los 10 segundos son una escala de demostración, no el intervalo de demanda de una tarifa eléctrica.

**Asignación de pines del ESP32.**

| Función | GPIO | Contrato de conexión |
| --- | ---: | --- |
| Tensión acondicionada, equivalente al ZMPT101B | 34 | Entrada ADC1_CH6. |
| Corriente acondicionada, equivalente al SCT013-000 | 35 | Entrada ADC1_CH7. |
| DS18B20 | 4 | Datos 1-Wire, con resistencia de 4,7 kΩ hacia 3,3 V. |
| PIR | 27 | Entrada digital; 1 representa movimiento detectado. |
| Mando de la carga no prioritaria | 26 | HIGH ordena conectar; LOW ordena desconectar. |
| Buzzer | 25 | Salida de tono; estado apagado sin bloqueo. |
| LED de alarma | 33 | HIGH enciende; resistencia limitadora de 330 Ω. |
| Rearme y consulta | USB/monitor serie | Comandos REARMAR y ESTADO; no requiere un pulsador adicional. |

GPIO34 y GPIO35 son entradas de ADC1 y no cuentan con resistencias internas de pull-up o pull-down. La asignación analógica evita ADC2 y conserva disponibilidad para la conectividad WiFi de la tercera fase. Los GPIO se refieren a la numeración del ESP32, no al número físico de una patilla.

El contrato de GPIO26 corresponde al conjunto previsto de NPN de interfaz y módulo relé de 5 V activo en bajo: HIGH activa el NPN y energiza el relé; LOW lo desenergiza y abre COM/NO. El modelo virtual representará ese comportamiento global. Al construirlo se comprobará la continuidad COM/NO y se ajustará la polaridad del modelo para cumplir el contrato. La posición del contacto determinará el estado de la rama desconectable en el modelo eléctrico.

**Representación de los sensores eléctricos.** Se utilizará un modelo virtual de salidas acondicionadas, identificado como tal. El modelo generará dos señales senoidales de 60 Hz con amplitud y desfase controlables mediante la API analógica y temporizadores de Wokwi. No se presentará como una simulación física interna del SCT013-000 o del ZMPT101B.

Las salidas se definen como `u_V(t) = 1,65 V + 0,05 × v(t)` y `u_I(t) = 1,65 V + 2,00 × i(t)`, donde v(t) está en voltios representados e i(t) en amperios representados. A 15 V RMS, u_V permanece aproximadamente entre 0,589 y 2,711 V. A 0,30 A RMS, u_I permanece entre 0,801 y 2,499 V. Estas ganancias pertenecen al modelo virtual; la calibración de los sensores físicos se realizará al implementar el hardware.

Se fijará una actualización objetivo del generador cada 100 microsegundos. En la etapa 2 se contrastarán puntos conocidos de entrada y lectura ADC para obtener la conversión efectiva del simulador antes de calcular magnitudes eléctricas. La documentación de la API de Wokwi describe una referencia virtual de 5 V, que no debe confundirse con la escala del ADC físico del ESP32; esta última depende de la atenuación y calibración. Las señales virtuales definidas mantienen amplitudes compatibles con la interfaz prevista de 3,3 V.

La corriente representada será la suma de las ramas efectivamente conectadas. Abrir el relé eliminará la contribución de la rama no prioritaria; no se reducirá solo una cifra mostrada por el programa. El modelo permitirá comprobar que la carga prioritaria conserva su alimentación.

**Cálculo de las magnitudes.** Se corregirá el offset de cada canal, se aplicarán sus factores de escala y se alinearán temporalmente las muestras. Se calcularán `V_RMS = raíz(media(v²))`, `I_RMS = raíz(media(i²))` y `P = media(v × i)`. La potencia aparente será `S = V_RMS × I_RMS`. Para cargas con desfase no se identificará S con P. La energía se acumulará integrando P con el tiempo real de simulación: con P en W y Δt en segundos, `ΔE_kWh = P × Δt / 3.600.000`.

**Estados y reglas de control.**

| Estado | Relé de carga no prioritaria | Alarma | Comportamiento |
| --- | --- | --- | --- |
| INICIO | Abierto | Apagada durante la inicialización | Obtener mediciones válidas. Un fallo o timeout lleva a BLOQUEADO. |
| ESPERA | Abierto | Apagada | Esperar una orden manual válida; no conectar automáticamente después del arranque. |
| NORMAL | Cerrado | Apagada | Medir y supervisar continuamente. |
| BLOQUEADO | Abierto | Activada | Conservar la causa del disparo o fallo y esperar condiciones y orden de rearme válidas. |

Durante NORMAL, si `P > 1,50 W` o `T > 55 °C`, se pasará a BLOQUEADO en la siguiente evaluación válida, sin retardo de disparo intencional. Un fallo de medición también abrirá el relé y activará la alarma. Las causas podrán acumularse: potencia, temperatura y fallo de sensor. La normalización de una lectura no reconectará la carga ni borrará automáticamente el bloqueo.

REARMAR solo se aceptará con datos recientes y válidos, `P < 1,50 W` y `T < 50 °C`. Si reaparece una causa de corte después del rearme, el sistema volverá a desconectar. A 50 °C exactos el rearme seguirá bloqueado. A 55 °C exactos no habrá un nuevo disparo causado exclusivamente por temperatura; un disparo anterior permanecerá retenido. El PIR informará movimiento y no podrá anular protecciones ni ordenar por sí solo el corte de la rama prioritaria.

Serán inválidas las lecturas con errores de comunicación o CRC, valores no finitos, saturación o valores fuera del rango definido, así como datos envejecidos. Para el DS18B20 se comprobará además su rango físico de -55 a 125 °C. Cero amperios o cero voltios representados no se considerarán por sí solos una prueba de sensor averiado; las entradas analógicas no permiten diagnosticar toda desconexión únicamente a partir de la magnitud calculada.

**Valores de referencia para la demostración.** Todos los casos siguientes utilizan 12 V RMS y cargas resistivas, excepto la referencia específica de desfase.

| Caso | R prioritaria | R no prioritaria y estado | I RMS total | P esperada |
| --- | ---: | --- | ---: | ---: |
| Normal, después de habilitar | 220 Ω | 220 Ω conectada | 0,10909 A | 1,30909 W |
| Sobrecarga, antes del corte | 220 Ω | 110 Ω conectada | 0,16364 A | 1,96364 W |
| Sobrecarga, después del corte | 220 Ω | Desconectada | 0,05455 A | 0,65455 W |
| Sobretemperatura | 220 Ω | Conexión normal y T = 60 °C | Pasa de 0,10909 a 0,05455 A | Pasa de 1,30909 a 0,65455 W |

Como referencia independiente del cálculo de potencia, con 12 V RMS, 0,10909 A RMS y desfase de 60 grados, se espera S = 1,30909 VA y P = 0,65455 W. Es un caso de señal para verificar el algoritmo, diferente del escenario resistivo normal. A potencia normal constante durante 60 segundos, la energía esperada es 0,0000218182 kWh.

Se comprobaron numéricamente estos valores con 200 muestras ideales por ventana. Una cuantización de referencia de 12 bits y escala de 5 V produjo errores de potencia inferiores a 0,02 % en esos casos. Este cálculo verifica las referencias matemáticas; no constituye una prueba de ejecución de Wokwi ni una precisión garantizada del hardware.

Para la futura simulación se proponen tolerancias de aceptación de ±máximo(1 %, 0,05 V) en tensión, ±máximo(1 %, 0,002 A) en corriente y ±máximo(1 %, 0,01 W) en potencia, en condiciones estacionarias de referencia. La energía se comprobará con una tolerancia del 2 % tras al menos 10 segundos estables. Las pruebas lógicas exactas de umbral se distinguirán de las pruebas analógicas sujetas a cuantización.

**Cierre de esta etapa.** Quedan definidos la plataforma, las señales, los rangos, la escala de potencia, los pines, las condiciones de deslastre y rearme, los estados y los casos numéricos de referencia. La etapa 2 construirá el circuito y verificará la escala ADC y la polaridad del relé con el modelo elegido; las etapas 3 a 5 implementarán y probarán el firmware. No se modifica el informe de la primera fase.

Fuentes del proyecto: Proyectos IoT, Tema 2, página 5; informe final de primera fase, páginas 3 y 5; Plan_Desarrollo_Fase_2_IoT.md.

Referencias técnicas consultadas el 28 de octubre de 2026:

- ESP32 GPIO y ADC1: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html
- API ADC Arduino-ESP32: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html
- Wokwi, API analógica: https://docs.wokwi.com/chips-api/analog
- Wokwi, temporizadores: https://docs.wokwi.com/chips-api/time
- Wokwi, módulo relé: https://docs.wokwi.com/parts/wokwi-relay-module
- Wokwi, DS18B20: https://docs.wokwi.com/parts/wokwi-ds18b20
- Analog Devices, DS18B20: https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf
