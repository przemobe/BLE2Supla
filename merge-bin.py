import os
from pathlib import Path
from SCons.Script import DefaultEnvironment
import shutil

env = DefaultEnvironment()

def merge_bin(source, target, env):

    my_flags = env.ParseFlags(env['BUILD_FLAGS'])
    defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
    ver = defines.get("APP_VERSION")


    build_dir = Path(env.subst("$BUILD_DIR"))
    output_bin = build_dir / "merged-firmware.bin"

    # Lista (offset, plik) do mergowania
    segments = [
        ("0x0000", build_dir / "bootloader.bin"),
        ("0x8000", build_dir / "partitions.bin"),
        ("0x10000", build_dir / "firmware.bin")
    ]

    # Komenda podstawowa
    esptool_cmd = [
        "esptool",
        "--chip", env["BOARD_MCU"],
        "merge-bin",
        "-o", str(output_bin),
        "--flash-mode", "dio",
        "--flash-freq", "40m",
        "--flash-size", "4MB",
    ]

    # Dodaj offsety i ścieżki plików
    for offset, filepath in segments:
        esptool_cmd.extend([offset, str(filepath)])

    # Wypisz komendę
    print(f"[merge_bin] Merging to {output_bin}")
    print("[merge_bin] Running command:")
    print(" ".join(esptool_cmd))

    # Wykonaj komendę
    ret = os.system(" ".join(esptool_cmd))
    if ret != 0:
        print("[merge_bin] esptool merge-bin failed!")
        env.Exit(1)

    os.makedirs("dist", exist_ok=True)
    shutil.copyfile(output_bin, "dist/BLE2SUPLA"+ver+"_"+env["PIOENV"]+".bin")

# Wywołaj po wygenerowaniu firmware.bin
env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin)
