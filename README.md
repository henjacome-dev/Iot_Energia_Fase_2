# Sistema IoT para eficiencia energética

Proyecto académico MEC452. Este repositorio reúne la **fase 2: programación e integración simulada**. El ESP32 mide potencia y energía y desconecta una rama no prioritaria por exceso de potencia, sobretemperatura o fallo de medición. El circuito se ejecuta en Wokwi.

| Material | Contenido |
| --- | --- |
| [Prototipo Wokwi](01_Prototipo_Wokwi/) | Circuito, firmware, chip de señales y pruebas del núcleo. |
| [Especificación técnica](02_Documentacion/Especificacion_Final.md) | Pines, umbrales, estados y valores esperados. |
| Caso de estudio Schneider EcoStruxure | [PDF](02_Documentacion/Etapa_6_Caso_Estudio_EcoStruxure_PME.pdf) · [Word editable](02_Documentacion/Etapa_6_Caso_Estudio_EcoStruxure_PME.docx). |
| [Guía de comandos](02_Documentacion/Guia_Comandos.md) | Órdenes del monitor serie, interpretación de respuestas y comandos de terminal. |


La alimentación de 12 V RMS y las señales equivalentes a SCT013-000 y ZMPT101B se representan en un modelo virtual. El corte de demostración se activa si la potencia supera 1,50 W o la temperatura supera 55 °C. El montaje físico y la conectividad MQTT corresponden a la siguiente fase.

## Pruebas y entrega académica

El [registro de la etapa 7](03_Validacion/Comprobacion_Etapa_7.txt) documenta 37 de 37 pruebas del núcleo correctas, además de las comprobaciones del modelo C. Para repetirlas en una terminal con Python 3, GCC y G++ instalados:

```sh
cd 01_Prototipo_Wokwi
python3 verificacion/ejecutar.py
```

[Huellas SHA-256 de esta copia](MANIFIESTO_SHA256.txt).
