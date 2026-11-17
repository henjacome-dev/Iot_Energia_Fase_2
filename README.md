# Sistema IoT para eficiencia energética

Proyecto académico MEC452. Este repositorio reúne la **fase 2: programación e integración simulada**. El ESP32 mide potencia y energía y desconecta una rama no prioritaria por exceso de potencia, sobretemperatura o fallo de medición. El circuito se ejecuta en Wokwi.

| Material | Contenido |
| --- | --- |
| [Prototipo Wokwi](01_Prototipo_Wokwi/) | Circuito, firmware, chip de señales y pruebas del núcleo. |
| [Especificación técnica](02_Documentacion/Especificacion_Final.md) | Pines, umbrales, estados y valores esperados. |
| Caso de estudio Schneider EcoStruxure | [PDF](02_Documentacion/Etapa_6_Caso_Estudio_EcoStruxure_PME.pdf) · [Word editable](02_Documentacion/Etapa_6_Caso_Estudio_EcoStruxure_PME.docx). |
| [Validación](03_Validacion/Estado_Validacion.md) | Matriz y registros disponibles. |
| [Guía de comandos](02_Documentacion/Guia_Comandos.md) | Órdenes del monitor serie, interpretación de respuestas y comandos de terminal. |
| [Guion de demostración](04_Demostracion/Guion_Demo.md) | Pasos de exposición, ensayos y explicación de resultados. |

## Ejecutar la simulación

1. Abre un [proyecto ESP32 de Wokwi](https://wokwi.com/projects/new/esp32).
2. Copia `sketch.ino` y `diagram.json` de la carpeta [01_Prototipo_Wokwi](01_Prototipo_Wokwi/) en las pestañas correspondientes. Carga `energia_control.h`, `libraries.txt`, `medidor-ac.chip.c` y `medidor-ac.chip.json` en la raíz del proyecto Wokwi.
3. Inicia la simulación. Espera `ESPERA` y `MED=VALIDA`. Envía `REARMAR` en el monitor serie para conectar la rama no prioritaria.
4. Cambia la resistencia no prioritaria de 220 Ω a 110 Ω, o configura el DS18B20 a 60 °C, para comprobar el corte y la alarma. Sigue el [guion](04_Demostracion/Guion_Demo.md) para demostrar la retención del bloqueo y el rearme.

La alimentación de 12 V RMS y las señales equivalentes a SCT013-000 y ZMPT101B se representan en un modelo virtual. El corte de demostración se activa si la potencia supera 1,50 W o la temperatura supera 55 °C. El montaje físico y la conectividad MQTT corresponden a la siguiente fase.

## Pruebas y entrega académica

El [registro de la etapa 7](03_Validacion/Comprobacion_Etapa_7.txt) documenta 37 de 37 pruebas del núcleo correctas, además de las comprobaciones del modelo C. Para repetirlas en una terminal con Python 3, GCC y G++ instalados:

```sh
cd 01_Prototipo_Wokwi
python3 verificacion/ejecutar.py
```

La rúbrica de la segunda fase asigna **7 puntos a la simulación** y **3 al caso de estudio**. La [matriz incluida](03_Validacion/Matriz_Validacion_Etapa_5_IoT.xlsx) es la plantilla: incorpora tu copia completada y sus evidencias en [03_Validacion](03_Validacion/) antes de presentar el expediente. Si compartes la simulación en línea, añade aquí el enlace del proyecto Wokwi que hayas guardado y comprobado.

[Huellas SHA-256 de esta copia](MANIFIESTO_SHA256.txt).
