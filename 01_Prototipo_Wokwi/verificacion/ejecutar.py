"""Compila y prueba el mismo nucleo usado por el ESP32. Requiere GCC/G++."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parent
gcc, gpp = shutil.which("gcc"), shutil.which("g++")
if not gcc or not gpp:
    sys.exit("Se requieren gcc y g++ en PATH. Wokwi se ejecuta independientemente de esta prueba.")
salidas = []
with tempfile.TemporaryDirectory(prefix="iot_etapa4_") as temporal:
    for compilador, fuente, estandar, nombre in (
        (gcc, "prueba_modelo.c", "c11", "modelo"),
        (gpp, "pruebas_control.cpp", "c++20", "control"),
    ):
        binario = str(Path(temporal) / nombre)
        subprocess.run([compilador, f"-std={estandar}", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-pedantic", str(root / fuente), "-lm", "-o", binario], check=True)
        salida = subprocess.run([binario], text=True, capture_output=True)
        salidas.append(salida.stdout + salida.stderr)
        print(salidas[-1], end="")
        if salida.returncode:
            (root / "salida_pruebas.txt").write_text("\n".join(salidas), encoding="utf-8")
            sys.exit(salida.returncode)
(root / "salida_pruebas.txt").write_text("\n".join(salidas), encoding="utf-8")
