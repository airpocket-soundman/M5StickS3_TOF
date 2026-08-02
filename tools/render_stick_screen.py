"""Render the current M5StickS3 screen without the LVGL web simulator.

The geometry and text follow src/main.cpp on the main branch.  The graph is
reconstructed from the measured 500 Hz displacement trace stored in
img/grain_real_hand.svg, then sampled to the firmware's 240-point history.
"""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw


WIDTH = 240
HEIGHT = 135
HISTORY_SIZE = 240
GRAPH_Y = 48
GRAPH_SPAN_MM = 500.0
BASELINE_MM = 192.5

# Exact M5GFX Font0 (Adafruit classic 5x7), ASCII 0x20..0x7f.
FONT_BYTES = bytes.fromhex(
    "000000000000005F00000007000700147F147F14242A7F2A1223130864623649562050"
    "0008070300001C2241000041221C002A1C7F1C2A08083E0808008070300008080808080"
    "00060600020100804023E5149453E00427F400072494949462141494D331814127F1027"
    "454545393C4A49493141211109073649494936464949291E000014000000403400000008"
    "1422411414141414004122140802015909063E415D594E7C1211127C7F494949363E4141"
    "41227F4141413E7F494949417F090909013E414151737F0808087F00417F41002040413F"
    "017F081422417F404040407F021C027F7F0408107F3E4141413E7F090909063E4151215E"
    "7F09192946264949493203017F01033F4040403F1F2040201F3F4038403F631408146303"
    "047804036159494D43007F4141410204081020004141417F040201020440404040400003"
    "07080020545478407F284444383844444428384444287F385454541800087E090218A4A4"
    "9C787F0804047800447D40002040403D007F1028440000417F40007C047804787C080404"
    "783844444438FC1824241818242418FC7C08040408485454542404043F44243C4040207C"
    "1C2040201C3C4030403C44281028444C9090907C4464544C440008364100000077000000"
    "413608000201020402"
)


def rgb565(r: int, g: int, b: int) -> tuple[int, int, int]:
    """Round-trip the firmware color through RGB565 like the LCD."""
    value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    r5 = (value >> 11) & 0x1F
    g6 = (value >> 5) & 0x3F
    b5 = value & 0x1F
    return (
        (r5 << 3) | (r5 >> 2),
        (g6 << 2) | (g6 >> 4),
        (b5 << 3) | (b5 >> 2),
    )


COLOR_BG = rgb565(0, 0, 0)
COLOR_TEXT = rgb565(240, 240, 240)
COLOR_GRID = rgb565(48, 48, 48)
COLOR_DISTANCE = rgb565(0, 220, 120)
COLOR_TIMEOUT = rgb565(220, 60, 60)


def draw_font0_text(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int],
    text: str,
    foreground: tuple[int, int, int] = COLOR_TEXT,
    background: tuple[int, int, int] = COLOR_BG,
) -> None:
    """Draw opaque M5GFX Font0 glyph cells (6 x 8 pixels)."""
    cursor_x, cursor_y = xy
    origin_x = cursor_x
    for character in text:
        if character == "\n":
            cursor_x = origin_x
            cursor_y += 8
            continue
        code = ord(character)
        if not 32 <= code <= 127:
            code = ord("?")
        draw.rectangle((cursor_x, cursor_y, cursor_x + 5, cursor_y + 7), fill=background)
        offset = (code - 32) * 5
        for column in range(5):
            bits = FONT_BYTES[offset + column]
            for row in range(8):
                if bits & (1 << row):
                    draw.point((cursor_x + column, cursor_y + row), fill=foreground)
        cursor_x += 6


def measured_history(repo: Path, start_sample: int = 900) -> list[float]:
    """Read real displacement data and convert it back from SVG coordinates."""
    svg = subprocess.check_output(
        ["git", "show", "main:img/grain_real_hand.svg"],
        cwd=repo,
        text=True,
        encoding="utf-8",
    )
    match = re.search(r'<polyline points="([^"]+)"', svg)
    if match is None:
        raise RuntimeError("grain_real_hand.svg does not contain a polyline")

    svg_y = [float(point.split(",")[1]) for point in match.group(1).split()]
    # The source plot maps -16.1..54.0 mm over its full 260 px height.
    displacement = [-16.1 + (260.0 - y) * (54.0 + 16.1) / 260.0 for y in svg_y]
    # The source trace is recorded at 500 Hz.  Match kGp2yHistoryDecimation=4:
    # 125 screen updates/s x 240 points = 1.92 s of real measured motion.
    history = [BASELINE_MM + displacement[start_sample + i * 4] for i in range(HISTORY_SIZE)]
    return history


def render(repo: Path, output: Path) -> None:
    history = measured_history(repo)
    latest = history[-1]
    image = Image.new("RGB", (WIDTH, HEIGHT), COLOR_BG)
    draw = ImageDraw.Draw(image)

    status_lines = (
        "TOF GP2Y0E03",
        "bus: Hat G43/G44",
        f"latest: {latest:.1f} mm ok",
        "valid: yes rate: 1000.0Hz 1.00ms",
        "env:13.8mm on:27 gr:22/0",
        "wifi:OK gate:ON peak:22640",
        "BtnA: re-detect  BtnB: spk off",
    )
    draw_font0_text(draw, (0, 0), "\n".join(status_lines))

    # Same order and coordinates as updateDisplay(): pending capture, then gate.
    draw.ellipse((217, 5, 235, 23), fill=COLOR_TIMEOUT)
    draw.ellipse((195, 5, 213, 23), fill=COLOR_DISTANCE)

    graph_height = HEIGHT - GRAPH_Y
    draw.rectangle((0, GRAPH_Y, WIDTH - 1, HEIGHT - 1), outline=COLOR_GRID)
    inner_x = 1
    inner_y = GRAPH_Y + 1
    inner_width = WIDTH - 2
    inner_height = graph_height - 2
    minimum = BASELINE_MM - GRAPH_SPAN_MM / 3.0
    maximum = BASELINE_MM + GRAPH_SPAN_MM * 2.0 / 3.0

    points: list[tuple[int, int]] = []
    for i, value in enumerate(history):
        value = min(max(value, minimum), maximum)
        x = inner_x + (i * (inner_width - 1)) // (HISTORY_SIZE - 1)
        y = inner_y + inner_height - 1 - int(
            ((value - minimum) * (inner_height - 1)) / GRAPH_SPAN_MM
        )
        points.append((x, y))
    draw.line(points, fill=COLOR_DISTANCE, width=1)

    draw_font0_text(draw, (4, GRAPH_Y + 2), f"mm {minimum:.1f}-{maximum:.1f}")
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output, optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("stick_screen.png"))
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[1]
    output = args.output if args.output.is_absolute() else repo / args.output
    render(repo, output)
    print(output)


if __name__ == "__main__":
    main()
