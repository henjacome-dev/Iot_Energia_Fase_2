# Guion de demostración de la segunda fase

Duración orientativa de exposición: 6 a 8 minutos, según la velocidad de simulación y la explicación. Usar el reloj del monitor para los intervalos técnicos; el tiempo simulado puede avanzar a otra velocidad respecto al real.

## Preparación

Abrir el proyecto con los seis archivos indicados en [README principal](../README.md#ejecutar-la-simulación). Usar los valores iniciales: 12 V RMS, modo 0, rama no prioritaria de 220 Ω, desfase de 0° y temperatura de 25 °C. Conservar GPIO14/SYNC, GPIO32/NO y `cpuFrequency: "16"`. Tener abierto el caso comercial en PDF y el monitor serie.

Si se reutiliza una sesión, restaurar explícitamente los controles antes de reiniciar: las posiciones modificadas de los componentes pueden conservarse. Verificar el audio del navegador para escuchar el buzzer.

## Secuencia que se muestra al docente

| Paso | Acción | Qué mostrar y explicar |
| --- | --- | --- |
| 1 Arranque | Iniciar y esperar datos válidos. Enviar `ESTADO`. | Transición INICIO a ESPERA. NO abierto, alarma apagada y rama prioritaria activa. La no prioritaria arranca desconectada. |
| 2 Habilitación | Enviar `REARMAR` y pulsar Enter. | NORMAL, relé conectado, ambas ramas activas y alarma apagada. Lecturas cercanas a 12 V, 0,10909 A y 1,30909 W, con Q=0 y MED=VALIDA. |
| 3 Sobrecarga | Cambiar en el chip la rama no prioritaria a 110 Ω. | Evento por POTENCIA, BLOQUEADO, NO abierto, LED rojo y buzzer activos. El LED azul se apaga y el verde permanece encendido. P posterior próxima a 0,65455 W. |
| 4 Retención | Restablecer 220 Ω, esperar lecturas válidas y consultar `ESTADO`. Después enviar `REARMAR`. | La normalización conserva el bloqueo. Solo la orden válida reconecta y apaga la alarma. |
| 5 Temperatura | Cambiar DS18B20 a 60 °C y esperar su lectura. | BLOQUEADO por TEMPERATURA y apertura de la rama no prioritaria. |
| 6 Rearme térmico | Ajustar a 50 °C, esperar que el monitor lo confirme y enviar `REARMAR`. Luego ajustar a 49 °C, esperar la lectura y repetir. | A 50 °C se rechaza. A 49 °C, con las demás condiciones válidas, se acepta. |
| 7 Presencia | En NORMAL, usar Simulate motion del PIR. | PIR cambia sin provocar ni anular el deslastre. La presencia aporta contexto de operación. |
| 8 Pérdida de datos | Enviar `PAUSA_ADC`, esperar más de 300 ms simulados y solicitar `REARMAR`. | Bloqueo SENSOR, MED=INVALIDA_O_VENCIDA y rearme rechazado. Las cifras eléctricas anteriores no representan una lectura vigente. |
| 9 Recuperación | Enviar `SIGUE_ADC`, esperar MED=VALIDA y temperatura válida, y enviar `REARMAR`. | La adquisición recuperada no reconecta automáticamente. El rearme válido permite NORMAL. |
| 10 Métricas | Mantener funcionamiento normal al menos diez segundos válidos consecutivos y consultar `ESTADO`. | D10 deja de ser INCOMPLETA, la energía aumenta y Dmax conserva el máximo de ventanas completas. |
| 11 Caso comercial | Abrir el informe, especialmente su tabla comparativa. | Explicar qué añaden los históricos, la instrumentación y la supervisión de PME. Proponer registro persistente sin añadir sensores. |

El disparo por sobrecarga puede ocurrir antes del siguiente resumen de un segundo. Mostrar el EVENTO: el resumen posterior ya presenta la potencia reducida. Para terminar en estado estable, conservar 220 Ω, temperatura inferior a 50 °C, adquisiciones activas y rearme aceptado. `PARAR` permite cerrar la demostración con la rama no prioritaria abierta.

## Comprobaciones numéricas complementarias

- Con señales de 12 V RMS y desfase de 60°, esperar aproximadamente S=1,30909 VA, P=0,65455 W y FP=0,5. Restaurar 0° al finalizar.
- Para energía, tomar E inicial y final y utilizar el tiempo válido transcurrido con potencia estable. A 1,30909 W durante 60 s, el incremento esperado es 0,0000218182 kWh. La energía absoluta también incluye el consumo previo de la sesión.
- D10 se compara con una referencia constante después de diez segundos completos en esa misma condición. Un máximo registrado en una trayectoria con cambios no equivale al valor de una carga constante.
- La frecuencia del buzzer de 2 kHz se verifica mediante la captura del analizador lógico prevista en E5-41; el tono audible por sí solo no mide su frecuencia.

## Respuestas para la defensa

**¿Dónde están el SCT y el ZMPT?** En esta fase se representan sus salidas acondicionadas mediante un modelo virtual identificado. DS18B20 y PIR son componentes virtuales de Wokwi. La calibración de los sensores físicos pertenece a la siguiente fase.

**¿Por qué 12 V y 1,50 W?** Son escalas de demostración. El umbral térmico de más de 55 °C proviene del enunciado; el límite de potencia se eligió para mostrar el corte con las ramas de carga del modelo.

**¿Se mide potencia activa o solo V por I?** Se calcula la media de los productos de V e I alineados. S se calcula por separado y el ensayo de 60° permite comprobar que no se confunden.

**¿Por qué el relé sigue abierto cuando baja la temperatura?** El bloqueo queda retenido y requiere una orden manual con datos vigentes, P menor de 1,50 W, T menor de 50 °C y contacto abierto.

**¿Qué ocurre si se solicita reconectar con la sobrecarga aún configurada?** Con el relé abierto, la potencia medida puede ser baja y el rearme se acepta. Al reconectar la rama de 110 Ω, la siguiente medición detecta la sobrecarga y vuelve a cortar. Por eso se restaura 220 Ω para una recuperación estable.

**¿La demanda de diez segundos prueba un ahorro de factura?** Es una ventana de demostración. La evaluación económica real requiere el intervalo y el método del contrato eléctrico y registros de operación.

**¿Qué aporta el PIR?** Informa presencia. No cambia la prioridad de las cargas ni permite anular la supervisión.

**¿Qué mejora aporta comparar con PME?** Priorizar un registro persistente de variables y eventos para analizar el comportamiento entre sesiones. La conectividad y el dashboard se desarrollarán en la tercera fase manteniendo el control local.
