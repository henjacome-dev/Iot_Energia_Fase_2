# Prototipo Wokwi de la segunda fase

La versión de ejecución es la de etapa 4 validada por el equipo en etapa 5. Sus seis archivos de ejecución se conservan idénticos, byte por byte, al paquete de origen.

Para importarlo, sigue el [README principal](../README.md#ejecutar-la-simulación). Para el ensayo completo, consultar `../04_Demostracion/Guion_Demo.md`. La especificación vigente se encuentra en `../02_Documentacion/Especificacion_Final.md`.

El rótulo ETAPA 4 del circuito y del monitor identifica la versión del firmware. Su conservación permite reconocer el código validado; la etapa 7 empaqueta esa versión.

Las comprobaciones locales se repiten con `python3 verificacion/ejecutar.py` desde esta carpeta. El ejecutor genera `verificacion/salida_pruebas.txt`. Los antecedentes de etapa 4 se guardan en `../03_Validacion/Antecedentes_Etapa_4/`, y la ejecución de empaquetado en `../03_Validacion/Comprobacion_Etapa_7.txt`.
