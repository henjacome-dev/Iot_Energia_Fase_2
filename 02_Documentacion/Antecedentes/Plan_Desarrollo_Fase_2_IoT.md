# Plan de desarrollo de la segunda fase del proyecto IoT energético

Fecha de planificación: 28 de octubre de 2026.

El objetivo es entregar un prototipo simulado de monitoreo y deslastre energético, junto con una comparación frente a Schneider Electric EcoStruxure Power Monitoring Expert. La rúbrica asigna 7 puntos a la simulación y 3 puntos al caso de estudio. Estas son las únicas ponderaciones oficiales; las etapas siguientes organizan nuestro trabajo.

La base es el diseño de la primera fase: ESP32, corriente y voltaje asociados al SCT013-000 y ZMPT101B, temperatura DS18B20, presencia PIR, un relé de carga no prioritaria y alarma con buzzer y LED. La maqueta representa una alimentación monofásica de 12 VCA. El control debe continuar localmente.

Avanzaremos por hitos: cada etapa termina cuando su resultado se puede revisar y reproducir. Las etapas 1 a 5 preparan la simulación; la etapa 6 desarrolla el caso comercial y puede avanzar desde que se cierre el alcance de la etapa 1. La etapa 7 reúne y revisa ambos entregables.

| Etapa | Trabajo previsto | Resultado y criterio de cierre |
| --- | --- | --- |
| 1. Definir el alcance | Fijar rangos de tensión y corriente, límite de potencia, ventana de demanda, asignación de pines y estados de control. Elegir la representación virtual de las señales eléctricas. | Ficha técnica breve con parámetros, unidades y condiciones de corte y rearme, coherente con la primera fase. |
| 2. Construir el circuito virtual | Preparar ESP32, las cuatro entradas, relé, buzzer, LED y dos ramas de carga: prioritaria y desconectable. Definir la correspondencia entre sensores reales y modelos virtuales. | Proyecto de simulación que inicia y permite variar cada entrada. Abrir el relé debe reducir la corriente y la potencia representadas, conservando la rama prioritaria. |
| 3. Programar adquisición y medición | Leer las entradas; corregir offset y escala; calcular valores RMS, potencia activa, energía acumulada y demanda media y máxima. Mostrar variables y estados por monitor serie. | Lecturas contrastadas con casos de referencia conocidos, con unidades y alcance de la simulación documentados. |
| 4. Programar deslastre y alarmas | Integrar corte por exceso de potencia o temperatura, alarma, bloqueo ante fallos y rearme manual condicionado. | El sistema responde correctamente a cada causa de corte y rechaza reconexiones que incumplan las condiciones. |
| 5. Ejecutar y registrar pruebas | Comprobar condiciones normales, límites, exceso de potencia, sobretemperatura, presencia, fallos y reconexión. Registrar entrada, resultado esperado y observado. | Matriz de pruebas reproducible, con evidencias y fallos corregidos. Esta matriz es nuestra evidencia de funcionamiento; no añade subpuntajes a la rúbrica. |
| 6. Elaborar el caso comercial | Analizar EcoStruxure Power Monitoring Expert con documentación oficial y compararlo con el prototipo realmente implementado. | Texto breve con problema industrial, arquitectura, funciones, comparación, limitaciones, mejora aplicable y fuentes. Evitar atribuir al prototipo funciones todavía no demostradas. |
| 7. Preparar la entrega | Revisar archivos, instrucciones, esquema, código, evidencias y comparación comercial. Ensayar la demostración completa desde un inicio limpio. | Simulación reproducible por otra persona y paquete con los dos entregables exigidos. |

**Decisiones técnicas que conservaremos.** La condición obligatoria es cortar la carga no prioritaria y activar la alarma si la potencia supera el límite configurado **o** la temperatura supera 55 °C. El enunciado no fija un límite numérico de potencia: lo elegiremos para la escala de la maqueta. Según nuestro diseño previo, el rearme será manual, con sensores válidos, potencia bajo el límite y temperatura inferior a 50 °C. Al iniciar o detectar un sensor inválido, la carga no prioritaria quedará desconectada. El PIR se leerá y mostrará; la falta de movimiento por sí sola no ordenará cortar una carga prioritaria.

**Alcance de la simulación.** La herramienta propuesta es Wokwi. Las señales del SCT013-000 y ZMPT101B se representarán mediante modelos virtuales identificados. Se buscará generar muestras de tensión y corriente con amplitud, offset y desfase controlables para comprobar el cálculo de potencia activa descrito en la primera fase. Los controles de prueba o potenciómetros no se presentarán como sensores SCT o ZMPT físicamente simulados. Si se utiliza una simplificación con valores RMS y carga resistiva, se explicitará el supuesto de factor de potencia igual a uno y su alcance limitado. La estrategia se resolverá y verificará antes de desarrollar todo el programa.

Fritzing queda como complemento opcional para documentar el montaje físico. Las mediciones y el control se demostrarán ejecutando el firmware en la simulación. La verificación real del acondicionamiento analógico y la calibración del hardware quedará para la implementación física.

**Pruebas previstas para cerrar la simulación.**

| Prueba | Resultado esperado |
| --- | --- |
| Arranque o reinicio | Relé de la rama no prioritaria abierto y reconexión bloqueada hasta cumplir condiciones y recibir orden manual. |
| Operación normal tras habilitación manual | Lecturas y unidades correctas, carga no prioritaria conectada y alarma sin condición de fallo. |
| Potencia por debajo, igual y por encima del límite | El corte por potencia se activa únicamente al superar el límite, conforme a la condición definida. |
| Temperatura de 55 °C y superior a 55 °C | A 55 °C no se dispara exclusivamente por temperatura; al superarla, abre el relé y activa la alarma. |
| Dos causas de corte simultáneas | La carga no prioritaria permanece desconectada y la alarma activa. |
| Cambios de presencia | La lectura PIR cambia y se registra, sin anular la protección ni cortar por sí sola la carga prioritaria. |
| Normalización sin orden manual | La rama no prioritaria continúa desconectada. |
| Solicitud de rearme con temperatura de 50 °C o mayor, exceso de potencia o sensor inválido | La reconexión se rechaza. |
| Solicitud de rearme con temperatura inferior a 50 °C, potencia bajo el límite y sensores válidos | La reconexión se permite; si reaparece una causa de corte, vuelve a desconectar. |
| Apertura del relé | La corriente y potencia representadas disminuyen y la carga prioritaria conserva su alimentación. |
| Casos numéricos de referencia | La potencia y la energía corresponden a las señales y al tiempo simulados dentro de una tolerancia definida y documentada. |

**Comparación comercial.** Se estudiarán medición y calidad de datos, arquitectura, alarmas, control y deslastre, históricos y visualización, comunicaciones, seguridad, escalabilidad y costos de implementación o licenciamiento cuando exista información verificable. Para funciones ausentes o no documentadas se indicará esa limitación. Se propondrá una mejora alcanzable sin ampliar innecesariamente el costo del prototipo.

**Paquete de entrega propuesto.** Proyecto o enlace de simulación reproducible; código fuente comentado; esquema y correspondencia de señales; instrucciones para ejecutar y reproducir las pruebas; matriz con evidencias; y análisis comparativo con fuentes. Son medios propuestos para demostrar los dos entregables oficiales. El enunciado no establece una extensión documental ni fecha concreta para esta fase.

El hardware real, la conexión MQTT con TLS y el dashboard completo pertenecen a la tercera fase. Este plan concentra la segunda fase en la integración y control local simulados y en el caso de estudio. El primer hito será la ficha técnica de la etapa 1.

Fuentes de trabajo: Proyectos IoT, Tema 2, página 5; informe de primera fase del proyecto, páginas 3 y 5; documentación oficial de Wokwi sobre hardware compatible y API analógica; y documentación oficial de Schneider Electric EcoStruxure Power Monitoring Expert.

Referencias técnicas para las siguientes etapas: https://docs.wokwi.com/getting-started/supported-hardware ; https://docs.wokwi.com/chips-api/analog ; https://www.se.com/us/en/product-range/65404-ecostruxure-power-monitoring-expert/ .
