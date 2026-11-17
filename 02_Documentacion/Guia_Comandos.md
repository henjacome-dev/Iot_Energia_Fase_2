# Guía de comandos del prototipo IoT

Esta guía reúne las **ocho órdenes del monitor serie** del prototipo ESP32 de la fase 2. Incluye el efecto de cada orden, la respuesta esperada y ejemplos para la demostración. Al final se indican los comandos de terminal para repetir las pruebas y subir el repositorio.

## Cómo enviar una orden

Inicia Wokwi y escribe la orden en el **monitor serie**, una por línea. Pulsa **Enter** para enviarla. El proyecto usa 115 200 bit/s y fin de línea LF. El firmware ignora mayúsculas y minúsculas, y elimina espacios al principio y al final: ` estado ` funciona igual que `ESTADO`. Las órdenes no llevan argumentos.

| Orden | Qué hace | Qué observar |
| --- | --- | --- |
| `AYUDA` | Muestra las ocho órdenes, los umbrales de corte y rearme y los modos del chip virtual. | Aparece el menú de ayuda. El estado de las salidas se mantiene. |
| `ESTADO` | Pide un reporte inmediato de las mediciones, la carga, las alarmas y sus causas. | Aparece una línea de diagnóstico sin esperar el reporte periódico de un segundo. |
| `REARMAR` | Solicita conectar la carga no prioritaria. Si el rearme es válido, pasa a `NORMAL`, borra las causas retenidas y apaga el LED rojo y el buzzer. | `REARME ACEPTADO`. Comprueba `RELE=1`, `NO=CERRADO` y `ALARMA=0` una vez asentado el contacto. Si se rechaza, el monitor explica el motivo. |
| `PARAR` | Ordena abrir el relé de la carga no prioritaria. Desde `NORMAL` pasa a `ESPERA`. Si ya está `BLOQUEADO`, conserva el bloqueo y su alarma. | `PARADA MANUAL: orden de apertura; consultar contacto NO.` Verifica `NO=ABIERTO` con `ESTADO`. La rama prioritaria continúa mientras exista alimentación representada. |
| `PAUSA_ADC` | Suspende el muestreo de tensión y corriente para ensayar un fallo. Interrumpe la ventana de demanda de 10 s y conserva el máximo de la sesión. | Cuando la última medición tiene más de **300 ms simulados**, aparece `MED=INVALIDA_O_VENCIDA`, causa `SENSOR`, apertura del relé y alarma. La energía deja de crecer sin ventanas válidas nuevas. |
| `SIGUE_ADC` | Reanuda el muestreo de tensión y corriente. | Espera una ventana nueva con `MED=VALIDA`. Si hubo bloqueo, envía `REARMAR` cuando se cumplan las condiciones. |
| `PAUSA_TEMP` | Suspende las actualizaciones del DS18B20 para ensayar la detección de temperatura vencida. El muestreo eléctrico sigue activo. | Tras más de **3 s simulados** desde la última lectura válida, aparece `T=NO_VALIDA_O_VENCIDA`, causa `SENSOR`, apertura del relé y alarma. |
| `SIGUE_TEMP` | Reanuda el sensor y solicita una nueva conversión de temperatura. | Espera que el monitor vuelva a mostrar una lectura numérica de `T`. Si hubo bloqueo, envía `REARMAR` cuando se cumplan las condiciones. |

Las órdenes de pausa confirman su recepción con un mensaje `ENSAYO:`. El bloqueo por vencimiento se ve después, al exceder la antigüedad permitida. Para verificarlo, sigue el tiempo `t` del monitor.

## Cuándo se acepta el rearme

En `ESPERA` o `BLOQUEADO`, `REARMAR` exige mediciones válidas y recientes, sistema listo, **P < 1,50 W**, **T < 50 °C** y contacto normalmente abierto sin cerrar (`NO=ABIERTO`). Los umbrales de disparo son **P > 1,50 W o T > 55 °C**. Por eso 50 °C exactos impiden rearmar y 55 °C exactos no generan un disparo térmico por sí solos.

| Respuesta del monitor | Qué significa y qué hacer |
| --- | --- |
| `REARME ACEPTADO` | Se ordenó conectar la rama no prioritaria. Confirma el cierre con `ESTADO`. |
| `REARME YA ESTABA NORMAL` | El sistema estaba en `NORMAL` al pasar las verificaciones. |
| `REARME RECHAZADO: datos invalidos, vencidos o sistema no listo` | Falta una lectura válida y reciente, o la adquisición no está lista. Reanuda el canal pausado o corrige el fallo y vuelve a consultar. |
| `REARME RECHAZADO: P debe ser menor que 1.50 W` | La potencia medida es igual o superior al límite de rearme. Reduce la carga y verifica la nueva lectura. |
| `REARME RECHAZADO: T debe ser menor que 50 C` | Espera a que el DS18B20 reporte una temperatura inferior a 50 °C. |
| `REARME RECHAZADO: el contacto debe estar abierto antes del rearme` | El relé reporta `NO=CERRADO` cuando se solicita el rearme. Revisa la realimentación del contacto en el circuito antes de repetir. |

**Para recuperar la carga tras la prueba de sobrecarga:** devuelve la resistencia no prioritaria de **110 Ω a 220 Ω antes de rearmar**. Con esa rama abierta, la potencia visible puede haber bajado a unos **0,655 W**. Si dejas 110 Ω, el sistema puede aceptar el rearme y volver a disparar al medir la carga conectada.

## Cómo leer ESTADO

| Campo | Interpretación |
| --- | --- |
| `t` y estado | Milisegundos simulados desde el inicio. `INICIO`: espera las primeras lecturas; `ESPERA`: relé abierto y a la espera de una orden; `NORMAL`: carga habilitada; `BLOQUEADO`: apertura y alarma por fallo o disparo. |
| `RELE` y `NO` | `RELE=1` ordena conectar; `RELE=0` ordena abrir. `NO` confirma el estado leído del contacto normalmente abierto. Una discrepancia durante más de 100 ms genera `CONTACTO`. |
| `ALARMA` y `PIR` | `ALARMA=1`: LED rojo y buzzer de 2 kHz. `PIR=1`: detección de movimiento; es informativa. |
| `V`, `I`, `P`, `S`, `FP` y `T` | Tensión y corriente RMS, potencia activa (W), aparente (VA), factor de potencia y temperatura (°C). |
| `MED`, `Q` y `fs` | `MED` indica si la lectura eléctrica es válida y reciente. `Q` es la calidad de la ventana: 0 sin fallos, 1 saturación, 2 offset, 4 muestreo y 8 rango; los códigos se suman si coinciden. `fs` es la frecuencia de muestreo observada en Hz. |
| `E`, `validos`, `D10` y `Dmax` | Energía acumulada (kWh), segundos eléctricos válidos, demanda media móvil de 10 s (W) y su máximo durante la sesión. `D10=INCOMPLETA` indica que aún faltan datos consecutivos. |
| `CAUSAS`, `omitidas` e `invalidas` | Causas retenidas del bloqueo (`POTENCIA`, `TEMPERATURA`, `SENSOR` y/o `CONTACTO`) y contadores de ventanas omitidas e inválidas. |

Comprueba siempre `MED`: durante `PAUSA_ADC` pueden seguir apareciendo cifras antiguas de V, I y P con `Q=0`, aunque ya estén vencidas. Para identificar el instante del disparo, busca la línea `EVENTO` con el cambio de estado y su causa.

## Secuencias para practicar

| Prueba | Pasos en orden |
| --- | --- |
| Arranque y parada | Con modo 0, 12 V RMS, 220 Ω, desfase 0° y DS18B20 a 25 °C, espera `ESPERA`. Envía `REARMAR`, comprueba `NORMAL` con `ESTADO` y luego envía `PARAR`. Consulta de nuevo para confirmar `ESPERA` y `NO=ABIERTO`. |
| Pérdida de medición eléctrica | En `NORMAL`, envía `PAUSA_ADC`. Espera `BLOQUEADO` por `SENSOR`. Prueba `REARMAR` durante la pausa: debe rechazarse. Envía `SIGUE_ADC`, espera `MED=VALIDA` y vuelve a enviar `REARMAR`. |
| Pérdida de temperatura | En `NORMAL`, envía `PAUSA_TEMP`. Espera a que venza la lectura y se active `SENSOR`. Envía `SIGUE_TEMP`, espera una T válida menor de 50 °C y envía `REARMAR`. |
| Corte térmico y rearme | En `NORMAL`, ajusta el DS18B20 a 60 °C. Tras el corte, ajústalo a 50 °C y confirma que se rechaza `REARMAR`. Usa 49 °C, espera la lectura y repite. |

Para ensayar otros comportamientos, sigue el [guion de demostración](../04_Demostracion/Guion_Demo.md) y la [especificación técnica](Especificacion_Final.md).

## Entradas que el monitor no admite

- Órdenes de versiones previas como `RELE 1`, `LED 0` o `BUZZER 0` reciben `Orden desconocida. Escriba AYUDA.`. Las salidas se gobiernan mediante la lógica de control.
- Una línea vacía se ignora. Una línea de más de 63 bytes se descarta completa y devuelve `Orden descartada: linea demasiado larga.`.
- La resistencia, el desfase, el voltaje, el modo del chip, la temperatura y el movimiento se cambian en los **controles de los componentes de Wokwi**. El chip usa modo 0 para CA; modos 1, 2 y 3 para niveles DC de 1,00 V, 1,65 V y 2,50 V; y modos 4 y 5 para ensayar saturación de tensión o corriente. Vuelve a modo 0 para operar el prototipo.

## Comandos de terminal para este repositorio

Ejecuta estos comandos en una terminal del computador. Para repetir las pruebas del modelo y del núcleo, necesitas Python 3, GCC y G++ en el PATH:

```sh
cd 01_Prototipo_Wokwi
python3 verificacion/ejecutar.py
```

`cd` entra a la carpeta del simulador. El script compila y ejecuta las pruebas y escribe `verificacion/salida_pruebas.txt`. En Windows puedes usar `py verificacion/ejecutar.py` desde esa misma carpeta si Python está instalado con el lanzador `py`.

Para subir el proyecto desde la terminal, trabaja en **la raíz del repositorio**, donde está el `README.md`. Si el repositorio remoto de GitHub es nuevo y está vacío, reemplaza `USUARIO/REPOSITORIO` por tu ruta:

| Comando | Qué hace |
| --- | --- |
| `git init -b main` | Inicializa Git localmente con la rama `main`. |
| `git status` | Muestra qué archivos cambiaron y cuáles se incluirán en el próximo commit. |
| `git add .` | Prepara los archivos y cambios de la carpeta para el commit, respetando las exclusiones configuradas. Revisa el resultado con `git status`. |
| `git commit -m "Proyecto IoT de eficiencia energética"` | Guarda una versión local con los cambios preparados y un mensaje descriptivo. |
| `git remote add origin https://github.com/USUARIO/REPOSITORIO.git` | Asocia el repositorio local con el remoto en GitHub. |
| `git push -u origin main` | Sube los commits de `main` y configura su relación con la rama remota. Para subidas posteriores desde esa rama basta `git push`. |
| `git clone https://github.com/USUARIO/REPOSITORIO.git` | Descarga un repositorio que ya existe junto con su historial. Si partes de este comando, continúa con `git status`, `git add`, `git commit` y `git push` desde la carpeta clonada. |

Referencias: [firmware](../01_Prototipo_Wokwi/sketch.ino), [lógica de control y parser](../01_Prototipo_Wokwi/energia_control.h), [monitor serie de Wokwi](https://docs.wokwi.com/guides/serial-monitor) y [carga de un proyecto con Git](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github).
