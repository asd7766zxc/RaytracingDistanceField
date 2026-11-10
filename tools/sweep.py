import argparse
import csv
import subprocess
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.backends.backend_pdf import PdfPages
from PIL import Image

REPO = Path(__file__).resolve().parent.parent
PROJECT = REPO / "RaytracingDistanceField"
DEFAULT_EXE = REPO / "x64" / "Release" / "RaytracingDistanceField.exe"


def parse_args():
    p = argparse.ArgumentParser(description="Headless parameter sweep with RMSE against the highest photon count point-mode render.")
    p.add_argument("--exe", type=Path, default=DEFAULT_EXE)
    p.add_argument("--scenes", nargs="+", default=["teapot", "prism"], choices=["teapot", "prism"])
    p.add_argument("--modes", nargs="+", default=["point", "quad"], choices=["point", "quad"])
    p.add_argument("--photons", nargs="+", type=int, default=[16384, 65536, 262144, 1048576, 4194304])
    p.add_argument("--wavelengths", type=int, default=3, choices=[1, 3])
    p.add_argument("--frames", type=int, default=30)
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--width", type=int, default=800)
    p.add_argument("--height", type=int, default=800)
    p.add_argument("--camera", default="batch/camera.txt")
    p.add_argument("--out", type=Path, default=PROJECT / "batch_out" / "sweep")
    p.add_argument("--skip-existing", action="store_true")
    return p.parse_args()


def run_one(args, scene, mode, photons):
    name = f"{scene}_{mode}_{photons}"
    cfg_path = args.out / "configs" / f"{name}.cfg"
    png_path = args.out / "images" / f"{name}.png"
    csv_path = args.out / "timings" / f"{name}.csv"
    if args.skip_existing and png_path.exists() and csv_path.exists():
        return png_path, csv_path
    cfg = {
        "scene": scene,
        "width": args.width,
        "height": args.height,
        "camera": args.camera,
        "photons": photons,
        "wavelengths": args.wavelengths,
        "splat": mode,
        "frames": args.frames,
        "seed": args.seed,
        "output": png_path.resolve(),
        "timing_csv": csv_path.resolve(),
    }
    cfg_path.write_text("".join(f"{k} = {v}\n" for k, v in cfg.items()), encoding="utf-8")
    print(f"[sweep] {name}", flush=True)
    result = subprocess.run([str(args.exe), "--batch", str(cfg_path.resolve())], cwd=PROJECT, capture_output=True, text=True)
    if result.returncode != 0 or not png_path.exists():
        sys.exit(f"render failed for {name} (exit {result.returncode}):\n{result.stdout[-2000:]}\n{result.stderr[-2000:]}")
    return png_path, csv_path


def load_image(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.float64) / 255.0


def mean_timings(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if len(rows) > 1:
        rows = rows[1:]
    keys = ["photon_ms", "splat_ms", "raytrace_ms"]
    return {k: float(np.mean([float(r[k]) for r in rows])) for k in keys}


def main():
    args = parse_args()
    if not args.exe.exists():
        sys.exit(f"executable not found: {args.exe}")
    for sub in ("configs", "images", "timings"):
        (args.out / sub).mkdir(parents=True, exist_ok=True)
    photons = sorted(set(args.photons))
    ref_photons = photons[-1]

    records = []
    for scene in args.scenes:
        renders = {}
        for mode in sorted(set(args.modes) | {"point"}):
            for n in photons:
                if mode not in args.modes and n != ref_photons:
                    continue
                renders[(mode, n)] = run_one(args, scene, mode, n)
        reference = load_image(renders[("point", ref_photons)][0])
        for (mode, n), (png, timing) in sorted(renders.items()):
            if mode not in args.modes:
                continue
            rmse = float(np.sqrt(np.mean((load_image(png) - reference) ** 2)))
            t = mean_timings(timing)
            records.append({
                "scene": scene,
                "mode": mode,
                "photons": n,
                "rays": n if mode == "point" else max(2, round(n ** 0.5)) ** 2,
                "rmse": rmse,
                "is_reference": mode == "point" and n == ref_photons,
                **t,
                "caustic_ms": t["photon_ms"] + t["splat_ms"],
                "total_ms": t["photon_ms"] + t["splat_ms"] + t["raytrace_ms"],
                "image": str(png),
            })

    summary = args.out / "summary.csv"
    with open(summary, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(records[0].keys()))
        w.writeheader()
        w.writerows(records)

    pdf_path = args.out / "sweep.pdf"
    markers = {"point": "o", "quad": "s"}
    with PdfPages(pdf_path) as pdf:
        fig, axes = plt.subplots(len(args.scenes), 2, figsize=(11, 4.2 * len(args.scenes)), squeeze=False)
        for row, scene in enumerate(args.scenes):
            ax_err, ax_time = axes[row]
            for mode in args.modes:
                rs = sorted((r for r in records if r["scene"] == scene and r["mode"] == mode), key=lambda r: r["photons"])
                err = [r for r in rs if not r["is_reference"]]
                if err:
                    ax_err.loglog([r["rays"] for r in err], [r["rmse"] for r in err], marker=markers[mode], label=mode)
                ax_time.loglog([r["rays"] for r in rs], [r["caustic_ms"] for r in rs], marker=markers[mode], label=f"{mode} caustic pass")
                ax_time.loglog([r["rays"] for r in rs], [r["raytrace_ms"] for r in rs], marker=markers[mode], linestyle="--", label=f"{mode} raytrace pass")
            ax_err.set_title(f"{scene}: RMSE vs reference (point, {ref_photons} photons)")
            ax_err.set_xlabel("light rays per frame")
            ax_err.set_ylabel("RMSE")
            ax_time.set_title(f"{scene}: GPU time per frame")
            ax_time.set_xlabel("light rays per frame")
            ax_time.set_ylabel("ms")
            for ax in (ax_err, ax_time):
                ax.grid(True, which="both", alpha=0.3)
                ax.legend(fontsize=8)
        fig.suptitle(f"{args.frames} frames, seed {args.seed}, {args.wavelengths} wavelength(s), {args.width}x{args.height}")
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

        cols = [("point", ref_photons, "reference")] + [(m, photons[0], f"{m} {photons[0]}") for m in args.modes] + [(m, ref_photons, f"{m} {ref_photons}") for m in args.modes if m != "point"]
        fig, axes = plt.subplots(len(args.scenes), len(cols), figsize=(3.2 * len(cols), 3.9 * len(args.scenes)), squeeze=False)
        for row, scene in enumerate(args.scenes):
            lookup = {(r["mode"], r["photons"]): r for r in records if r["scene"] == scene}
            for col, (mode, n, label) in enumerate(cols):
                ax = axes[row][col]
                ax.axis("off")
                r = lookup.get((mode, n))
                if r is None:
                    continue
                ax.imshow(Image.open(r["image"]).convert("RGB"))
                ax.set_title(f"{scene}: {label}" + ("" if r["is_reference"] else f"\nRMSE {r['rmse']:.4f}"), fontsize=8)
        fig.tight_layout()
        pdf.savefig(fig)
        plt.close(fig)

    print(f"[sweep] wrote {summary} and {pdf_path}")


if __name__ == "__main__":
    main()
