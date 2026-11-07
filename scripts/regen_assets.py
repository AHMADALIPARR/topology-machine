#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Topology Machine contributors
"""Regenerate README figures from recorded tm and kg output.

Images of command output are transcript renders (DejaVu Sans Mono on a
dark panel). They are not photographs. Charts use the bench file and
the probabilities printed by `tm qasm sim`.
"""

from __future__ import annotations

import statistics
import subprocess
import textwrap
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
TM = ROOT / "build" / "tm"
KG = ROOT / "third_party" / "klong" / "kg"
ASSETS = ROOT / "assets"


def run(args: list[str]) -> str:
    proc = subprocess.run(
        args, cwd=ROOT, check=True, text=True, capture_output=True
    )
    return proc.stdout


def wrap_block(text: str, width: int = 140) -> list[str]:
    lines: list[str] = []
    for raw in text.splitlines() or [""]:
        if len(raw) <= width:
            lines.append(raw)
        else:
            lines.extend(textwrap.wrap(raw, width) or [""])
    return lines


def render_transcript(path: Path, sections: list[tuple[str, str]], title: str) -> None:
    """Write an SVG whose text is the recorded command output."""
    rows: list[tuple[str, str]] = []
    for command, output in sections:
        rows.append(("cmd", f"$ {command}"))
        for line in wrap_block(output.rstrip("\n")):
            rows.append(("out", line))
        rows.append(("gap", ""))
    caption = "SVG text of recorded command output, not a photograph."
    longest = max([len(title), len(caption)] + [len(text) for _, text in rows])
    width = max(720, 48 + longest * 8)
    line_h = 18
    height = 64 + line_h * (len(rows) + 2)
    body = [
        f'<text x="24" y="28" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#7dcea0">{escape(title)}</text>',
        f'<text x="24" y="48" font-family="DejaVu Sans, sans-serif" font-size="12" fill="#f0c674">{escape(caption)}</text>',
    ]
    y = 74
    for kind, text in rows:
        color = "#f0c674" if kind == "cmd" else "#e7ecf3"
        body.append(
            f'<text x="24" y="{y}" font-family="DejaVu Sans Mono, monospace" font-size="13" fill="{color}">{escape(text)}</text>'
        )
        y += line_h
    path.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" role="img">
  <title>{escape(title)}</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <rect x="8" y="8" width="{width - 16}" height="{height - 16}" fill="#22262e" stroke="#3c4452"/>
  {''.join(body)}
</svg>
''',
        encoding="utf-8",
    )


def badge(path: Path, left: str, right: str, right_rgb: tuple[int, int, int]) -> None:
    left_w = 8 * len(left) + 18
    right_w = 8 * len(right) + 18
    height = 28
    path.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="{left_w + right_w}" height="{height}" role="img">
  <title>{escape(left)}: {escape(right)}</title>
  <rect width="{left_w}" height="{height}" fill="#30343c"/>
  <rect x="{left_w}" width="{right_w}" height="{height}" fill="#{right_rgb[0]:02x}{right_rgb[1]:02x}{right_rgb[2]:02x}"/>
  <text x="{left_w / 2}" y="18" text-anchor="middle" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#e6eaf0">{escape(left)}</text>
  <text x="{left_w + right_w / 2}" y="18" text-anchor="middle" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#ffffff">{escape(right)}</text>
</svg>
''',
        encoding="utf-8",
    )


def parse_bench(text: str) -> dict[str, list[float]]:
    series: dict[str, list[float]] = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) != 4 or parts[0] == "name" or parts[0].startswith("#"):
            continue
        if parts[0] == "sink":
            continue
        try:
            ns = float(parts[3])
        except ValueError:
            continue
        series.setdefault(parts[0], []).append(ns)
    return series


def chart_bench(path: Path, series: dict[str, list[float]]) -> None:
    names = list(series)
    medians = [statistics.median(series[n]) for n in names]
    max_v = max(medians) if medians else 1.0
    rows = []
    svg_h = 80 + 48 * len(names)
    for i, (name, med) in enumerate(zip(names, medians)):
        y = 70 + i * 48
        bar_w = (640 * (med / max_v)) if max_v else 0
        rows.append(
            f'<text x="24" y="{y + 18}" font-family="DejaVu Sans Mono, monospace" font-size="14" fill="#e7ecf3">{escape(name)}</text>'
            f'<rect x="340" y="{y}" width="{bar_w:.1f}" height="22" fill="#7dcea0"/>'
            f'<text x="{348 + bar_w:.1f}" y="{y + 16}" font-family="DejaVu Sans Mono, monospace" font-size="14" fill="#f0c674">{med:.3f} ns</text>'
        )
    path.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="980" height="{svg_h}" role="img">
  <title>Benchmark medians from bench/results/bench-runs.txt</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <text x="24" y="32" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">Nanoseconds per call, median of 5 raw runs</text>
  <text x="24" y="52" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#9aa4b2">Derived from recorded bench output on this host. Not a hardware rating.</text>
  {''.join(rows)}
</svg>
''',
        encoding="utf-8",
    )


def chart_bell(path: Path, probs: list[tuple[str, float]]) -> None:
    bars = []
    for i, (label, p) in enumerate(probs):
        x = 80 + i * 160
        h = 200 * p
        bars.append(
            f'<rect x="{x}" y="{300 - h:.1f}" width="70" height="{max(h, 1):.1f}" fill="#7dcea0"/>'
            f'<text x="{x + 8}" y="324" font-family="DejaVu Sans Mono, monospace" font-size="14" fill="#e7ecf3">{escape(label)}</text>'
            f'<text x="{x}" y="{292 - h:.1f}" font-family="DejaVu Sans Mono, monospace" font-size="14" fill="#f0c674">{p:.3f}</text>'
        )
    path.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="760" height="360" role="img">
  <title>Bell probabilities from tm qasm sim</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <text x="32" y="28" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">Bell circuit probabilities from tm qasm sim</text>
  <text x="32" y="48" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#9aa4b2">Pre-measurement |amplitude|^2. This simulator does not sample collapse.</text>
  {''.join(bars)}
</svg>
''',
        encoding="utf-8",
    )


def architecture_svg() -> str:
    boxes = [
        (40, 70, 200, 70, "tm CLI", "commands, exit codes"),
        (300, 40, 220, 56, "Boolean DAG", "share, fold, eval"),
        (300, 110, 220, 56, "GF(2^8) + netlist", "AES poly 0x11b"),
        (300, 180, 220, 56, "Reversible gates", "CX, CCX, CSWAP"),
        (300, 250, 220, 56, "QASM subset sim", "≤ 4 qubits"),
        (300, 320, 220, 56, "Nodal solve", "dense G V = I"),
        (580, 70, 220, 70, "Text artifacts", "qasm, smt2, tmnl"),
        (580, 180, 220, 80, "Klong kg", "oracle.kg executed"),
        (580, 290, 220, 70, "Compare", "diff against tm oracle"),
    ]
    rects = []
    for x, y, w, h, title, sub in boxes:
        rects.append(
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="#22262e" stroke="#3c4452"/>'
            f'<text x="{x + 14}" y="{y + 28}" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">{escape(title)}</text>'
            f'<text x="{x + 14}" y="{y + 50}" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#9aa4b2">{escape(sub)}</text>'
        )
    arrows = [
        (240, 105, 300, 68),
        (240, 105, 300, 138),
        (240, 105, 300, 208),
        (240, 105, 300, 278),
        (240, 105, 300, 348),
        (520, 68, 580, 100),
        (520, 138, 580, 105),
        (690, 250, 690, 290),
    ]
    lines = []
    for x1, y1, x2, y2 in arrows:
        lines.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#7dcea0" stroke-width="2"/>')
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="860" height="430" role="img">
  <title>Topology Machine architecture</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <text x="40" y="32" font-family="DejaVu Sans, sans-serif" font-size="18" fill="#e7ecf3">Topology Machine</text>
  {''.join(lines)}
  {''.join(rects)}
</svg>
'''


def resistor_svg(v_line: str, residual_line: str) -> str:
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="760" height="420" role="img">
  <title>Two-node resistor network and solved voltages</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <text x="32" y="32" font-family="DejaVu Sans, sans-serif" font-size="18" fill="#e7ecf3">Reference network solved by tm analog</text>
  <text x="32" y="54" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#9aa4b2">Conductance matrix [[3,-1],[-1,2]], current [1,0]. Not a physical-lab photograph.</text>
  <line x1="80" y1="300" x2="640" y2="300" stroke="#9aa4b2" stroke-width="2"/>
  <line x1="80" y1="300" x2="70" y2="314" stroke="#9aa4b2"/>
  <line x1="100" y1="300" x2="90" y2="314" stroke="#9aa4b2"/>
  <line x1="120" y1="300" x2="110" y2="314" stroke="#9aa4b2"/>
  <text x="80" y="340" font-family="DejaVu Sans, sans-serif" font-size="14" fill="#9aa4b2">ground</text>
  <circle cx="220" cy="160" r="28" fill="#22262e" stroke="#7dcea0" stroke-width="2"/>
  <text x="208" y="165" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">A</text>
  <circle cx="520" cy="160" r="28" fill="#22262e" stroke="#7dcea0" stroke-width="2"/>
  <text x="508" y="165" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">B</text>
  <line x1="248" y1="160" x2="492" y2="160" stroke="#f0c674" stroke-width="3"/>
  <text x="330" y="150" font-family="DejaVu Sans, sans-serif" font-size="14" fill="#f0c674">1 Ω (1 S)</text>
  <line x1="220" y1="188" x2="220" y2="300" stroke="#f0c674" stroke-width="3"/>
  <text x="230" y="250" font-family="DejaVu Sans, sans-serif" font-size="14" fill="#f0c674">0.5 Ω (2 S)</text>
  <line x1="520" y1="188" x2="520" y2="300" stroke="#f0c674" stroke-width="3"/>
  <text x="530" y="250" font-family="DejaVu Sans, sans-serif" font-size="14" fill="#f0c674">1 Ω (1 S)</text>
  <line x1="140" y1="120" x2="192" y2="160" stroke="#7dcea0" stroke-width="3"/>
  <text x="40" y="110" font-family="DejaVu Sans, sans-serif" font-size="14" fill="#7dcea0">1 A into A</text>
  <text x="32" y="380" font-family="DejaVu Sans Mono, monospace" font-size="15" fill="#e7ecf3">{escape(v_line)}</text>
  <text x="32" y="402" font-family="DejaVu Sans Mono, monospace" font-size="15" fill="#e7ecf3">{escape(residual_line)}</text>
</svg>
'''


def bit0_svg(net_text: str) -> str:
    """Draw the generated y0 XOR chain, which is one output bit of the multiplier."""
    gates: dict[str, tuple[str, str, str]] = {}
    y0 = ""
    for line in net_text.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[0] in {"and", "xor"}:
            gates[parts[3]] = (parts[0], parts[1], parts[2])
        elif len(parts) == 3 and parts[0] == "output" and parts[1] == "y0":
            y0 = parts[2]
    chain: list[str] = []
    cursor = y0
    seen = set()
    while cursor in gates and cursor not in seen and gates[cursor][0] == "xor":
        seen.add(cursor)
        chain.append(cursor)
        cursor = gates[cursor][1]
    chain.reverse()
    rows = []
    y = 70
    for name in chain[:12]:
        kind, a, b = gates[name]
        rows.append(
            f'<text x="32" y="{y}" font-family="DejaVu Sans Mono, monospace" font-size="13" fill="#9aa4b2">{escape(a)}</text>'
            f'<text x="280" y="{y}" font-family="DejaVu Sans Mono, monospace" font-size="13" fill="#f0c674">{escape(kind)}</text>'
            f'<text x="360" y="{y}" font-family="DejaVu Sans Mono, monospace" font-size="13" fill="#9aa4b2">{escape(b)}</text>'
            f'<text x="620" y="{y}" font-family="DejaVu Sans Mono, monospace" font-size="13" fill="#7dcea0">{escape(name)}</text>'
        )
        y += 22
    height = y + 24
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="900" height="{height}" role="img">
  <title>Generated GF multiplier, output bit y0 XOR chain</title>
  <rect width="100%" height="100%" fill="#16181d"/>
  <text x="32" y="28" font-family="DejaVu Sans, sans-serif" font-size="16" fill="#e7ecf3">Generated netlist excerpt, output bit y0</text>
  <text x="32" y="48" font-family="DejaVu Sans, sans-serif" font-size="13" fill="#9aa4b2">Taken from examples/gf256.tmnl. Left input of each XOR is the running sum.</text>
  {''.join(rows)}
</svg>
'''


def main() -> None:
    if not TM.is_file():
        raise SystemExit("regen_assets: build/tm is missing; run make")
    ASSETS.mkdir(parents=True, exist_ok=True)
    (ASSETS / "badges").mkdir(exist_ok=True)
    (ASSETS / "transcripts").mkdir(exist_ok=True)
    out_dir = ROOT / "build" / "asset-out"
    out_dir.mkdir(parents=True, exist_ok=True)

    self_out = run([str(TM), "self-check", "--out-dir", "build/asset-out"])
    truth = run([str(TM), "dag", "truth", "--expr", "xor(and(i0,i1),not(i0))", "--bits", "2"])
    gf = run([str(TM), "gf", "mul", "0x57", "0x83"])
    inv = run([str(TM), "gf", "inv", "0x57"])
    analog = run([str(TM), "analog", "solve", "--g", "3,-1;-1,2", "--i", "1,0"])
    qasm = run([str(TM), "qasm", "sim", "build/asset-out/bell.qasm"])
    smt = run([str(TM), "smt", "check"])
    rev = run([str(TM), "rev", "apply", "--width", "3", "--state", "5", "--prog", "cx 0 1; ccx 0 1 2"])
    arrays = run([str(KG), "-n", "-q", "examples/arrays.kg"]) if KG.is_file() else ""
    oracle = run([str(TM), "oracle"])
    klong = run([str(KG), "-n", "-q", "klong/oracle.kg"]) if KG.is_file() else ""
    z3 = subprocess.run(["z3", "tests/golden/reversible.smt2"], cwd=ROOT, text=True, capture_output=True)
    z3_text = (z3.stdout or z3.stderr or "z3 produced no output").strip() + "\n"

    transcripts = {
        "self-check.txt": self_out,
        "truth.txt": truth,
        "gf.txt": gf,
        "analog.txt": analog,
        "qasm-sim.txt": qasm,
        "smt-check.txt": smt,
        "arrays.txt": arrays,
        "oracle.txt": oracle,
        "z3.txt": z3_text,
    }
    for name, text in transcripts.items():
        (ASSETS / "transcripts" / name).write_text(text)

    render_transcript(
        ASSETS / "cli-self-check.svg",
        [("tm self-check --out-dir build/asset-out", self_out)],
        "Native self-check",
    )
    render_transcript(
        ASSETS / "cli-models.svg",
        [
            ("tm dag truth --expr 'xor(and(i0,i1),not(i0))' --bits 2", truth),
            ("tm gf mul 0x57 0x83", gf),
            ("tm gf inv 0x57", inv),
            ("tm rev apply --width 3 --state 5 --prog 'cx 0 1; ccx 0 1 2'", rev),
            ("tm analog solve --g '3,-1;-1,2' --i '1,0'", analog),
            ("tm qasm sim build/asset-out/bell.qasm", qasm),
            ("tm smt check", smt),
            ("kg -n -q examples/arrays.kg", arrays),
        ],
        "Model commands",
    )
    same = "oracle lines match\n" if oracle == klong else "ORACLE MISMATCH\n"
    render_transcript(
        ASSETS / "cli-klong.svg",
        [
            ("tm oracle", oracle),
            ("kg -n -q klong/oracle.kg", klong),
            ("diff tm-oracle kg-oracle", same),
        ],
        "Klong interpreter compared with tm oracle",
    )

    bench_path = ROOT / "bench" / "results" / "bench-runs.txt"
    if not bench_path.is_file():
        bench_path = ROOT / "bench" / "results" / "bench.txt"
    series = parse_bench(bench_path.read_text()) if bench_path.is_file() else {}
    if series:
        chart_bench(ASSETS / "bench-ns.svg", series)

    probs = []
    for line in qasm.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[0] == "prob":
            probs.append((parts[1], float(parts[2])))
    if probs:
        chart_bell(ASSETS / "bell-probabilities.svg", probs)

    analog_lines = [ln for ln in analog.splitlines() if ln]
    (ASSETS / "resistor-network.svg").write_text(
        resistor_svg(analog_lines[0] if analog_lines else "", analog_lines[1] if len(analog_lines) > 1 else "")
    )
    (ASSETS / "architecture.svg").write_text(architecture_svg())
    net = (ROOT / "examples" / "gf256.tmnl").read_text()
    (ASSETS / "gf-bit0.svg").write_text(bit0_svg(net))

    badge(ASSETS / "badges" / "license.svg", "license", "AGPL-3.0-only", (47, 111, 78))
    badge(ASSETS / "badges" / "c11.svg", "language", "C11", (47, 90, 130))
    badge(ASSETS / "badges" / "klong.svg", "klong", "20221212", (90, 78, 130))
    badge(ASSETS / "badges" / "qasm.svg", "openqasm", "3 subset", (47, 111, 78))
    z3_record = (
        subprocess.run(["z3", "--version"], cwd=ROOT, text=True, capture_output=True).stdout
        + "command: z3 tests/golden/reversible.smt2\n"
        + z3_text
    )
    (ASSETS / "transcripts" / "z3-version.txt").write_text(z3_record)
    (ROOT / "bench" / "results").mkdir(parents=True, exist_ok=True)
    (ROOT / "bench" / "results" / "z3-involution.txt").write_text(z3_record)
    print("regen_assets: wrote", ASSETS)


if __name__ == "__main__":
    main()
