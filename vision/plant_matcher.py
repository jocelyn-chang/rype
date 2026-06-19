#!/usr/bin/env python3
"""Simple plant matcher (no training required).

Match a query image against user-supplied reference photos for:
- aloe
- pothos

Reference folder structure:
  vision/reference/aloe/*.jpg
  vision/reference/pothos/*.jpg

Examples:
  python3 vision/plant_matcher.py predict-image --image ./query.jpg
  python3 vision/plant_matcher.py predict-camera --camera 0
    python3 vision/plant_matcher.py predict-health-camera --species aloe --camera 0 --json
    python3 vision/plant_matcher.py check-health
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

import cv2
import numpy as np

DEFAULT_LABELS = ["aloe", "pothos"]
HEALTHY_KEYWORDS = ("healthy", "good", "normal", "fresh")
UNHEALTHY_KEYWORDS = (
    "unhealthy",
    "sick",
    "diseased",
    "rot",
    "rotting",
    "yellow",
    "wilt",
    "wilting",
    "brown",
    "dead",
    "damage",
    "damaged",
)


@dataclass
class RefDescriptor:
    label: str
    path: Path
    hist: np.ndarray
    keypoints: List[cv2.KeyPoint]
    descriptors: np.ndarray | None


def compute_hist(image: np.ndarray) -> np.ndarray:
    resized = cv2.resize(image, (256, 256), interpolation=cv2.INTER_AREA)
    hsv = cv2.cvtColor(resized, cv2.COLOR_BGR2HSV)
    hist = cv2.calcHist([hsv], [0, 1], None, [36, 32], [0, 180, 0, 256])
    hist = cv2.normalize(hist, None).flatten().astype(np.float32)
    return hist


def compute_orb(image: np.ndarray) -> Tuple[List[cv2.KeyPoint], np.ndarray | None]:
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    orb = cv2.ORB_create(nfeatures=900)
    return orb.detectAndCompute(gray, None)


def load_references(ref_root: Path, labels: List[str]) -> List[RefDescriptor]:
    refs: List[RefDescriptor] = []
    for label in labels:
        folder = ref_root / label
        if not folder.exists():
            continue
        paths = sorted(folder.glob("*.jpg")) + sorted(folder.glob("*.jpeg")) + sorted(folder.glob("*.png"))
        for path in paths:
            img = cv2.imread(str(path))
            if img is None:
                continue
            kp, des = compute_orb(img)
            refs.append(
                RefDescriptor(
                    label=label,
                    path=path,
                    hist=compute_hist(img),
                    keypoints=kp,
                    descriptors=des,
                )
            )
    return refs


def hist_similarity(h1: np.ndarray, h2: np.ndarray) -> float:
    # Correlation is in [-1, 1]. We scale to [0, 1].
    corr = float(cv2.compareHist(h1, h2, cv2.HISTCMP_CORREL))
    return max(0.0, min(1.0, (corr + 1.0) * 0.5))


def orb_similarity(des1: np.ndarray | None, des2: np.ndarray | None) -> float:
    if des1 is None or des2 is None or len(des1) < 8 or len(des2) < 8:
        return 0.0
    bf = cv2.BFMatcher(cv2.NORM_HAMMING)
    matches = bf.knnMatch(des1, des2, k=2)
    good = 0
    for pair in matches:
        if len(pair) != 2:
            continue
        m, n = pair
        if m.distance < 0.75 * n.distance:
            good += 1
    return float(good) / float(max(1, min(len(des1), len(des2))))


def predict(query_img: np.ndarray, refs: List[RefDescriptor]) -> Dict[str, object]:
    if not refs:
        raise RuntimeError("No valid reference photos found.")

    q_hist = compute_hist(query_img)
    _, q_des = compute_orb(query_img)

    scores: Dict[str, List[float]] = {}
    best_score = -1.0
    best_label = "unknown"
    best_ref = ""

    for ref in refs:
        hs = hist_similarity(q_hist, ref.hist)
        os = orb_similarity(q_des, ref.descriptors)

        # Weighted blend: color/texture distribution + local pattern matching.
        total = 0.65 * hs + 0.35 * os

        scores.setdefault(ref.label, []).append(total)
        if total > best_score:
            best_score = total
            best_label = ref.label
            best_ref = str(ref.path)

    label_scores = {
        label: float(np.mean(values)) for label, values in scores.items() if values
    }

    all_vals = list(label_scores.values())
    confidence = 0.0
    if all_vals:
        s = float(np.sum(all_vals)) + 1e-8
        confidence = float(label_scores.get(best_label, 0.0) / s)

    return {
        "label": best_label,
        "confidence": round(confidence, 4),
        "best_reference": best_ref,
        "label_scores": {k: round(v, 4) for k, v in sorted(label_scores.items())},
    }


def parse_labels(raw: str) -> List[str]:
    return [part.strip() for part in raw.split(",") if part.strip()]


def infer_health_from_filename(path: Path) -> str | None:
    name = path.stem.lower()
    if any(word in name for word in UNHEALTHY_KEYWORDS):
        return "unhealthy"
    if any(word in name for word in HEALTHY_KEYWORDS):
        return "healthy"
    return None


def load_species_health_references(ref_root: Path, species: str) -> List[RefDescriptor]:
    folder = ref_root / species
    if not folder.exists():
        raise RuntimeError(f"Species folder not found: {folder}")

    refs: List[RefDescriptor] = []
    paths = sorted(folder.glob("*.jpg")) + sorted(folder.glob("*.jpeg")) + sorted(folder.glob("*.png"))
    for path in paths:
        health_label = infer_health_from_filename(path)
        if health_label is None:
            continue
        img = cv2.imread(str(path))
        if img is None:
            continue
        kp, des = compute_orb(img)
        refs.append(
            RefDescriptor(
                label=health_label,
                path=path,
                hist=compute_hist(img),
                keypoints=kp,
                descriptors=des,
            )
        )

    if not refs:
        raise RuntimeError(
            "No health-labeled references found. Include 'healthy' or 'unhealthy' "
            "(or words like 'rot', 'yellow', 'diseased') in file names."
        )
    return refs


def read_camera_frame(camera_index: int) -> np.ndarray:
    cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open camera index {camera_index}.")
    ok, frame = cap.read()
    cap.release()
    if not ok or frame is None:
        raise RuntimeError("Could not capture frame from camera.")
    return frame


def cmd_predict_image(args: argparse.Namespace) -> int:
    refs = load_references(Path(args.refs), parse_labels(args.labels))
    query = cv2.imread(args.image)
    if query is None:
        print("ERROR: Could not read query image")
        return 1
    result = predict(query, refs)
    print(json.dumps(result) if args.json else f"Prediction: {result['label']} ({result['confidence']:.3f})")
    if not args.json:
        print(f"Best reference: {result['best_reference']}")
        print(f"Scores: {result['label_scores']}")
    return 0


def cmd_predict_camera(args: argparse.Namespace) -> int:
    refs = load_references(Path(args.refs), parse_labels(args.labels))
    query = read_camera_frame(args.camera)
    result = predict(query, refs)
    print(json.dumps(result) if args.json else f"Prediction: {result['label']} ({result['confidence']:.3f})")
    if not args.json:
        print(f"Best reference: {result['best_reference']}")
        print(f"Scores: {result['label_scores']}")
    return 0


def print_health_result(species: str, result: Dict[str, object], as_json: bool) -> None:
    payload = {
        "species": species,
        "health": result["label"],
        "confidence": result["confidence"],
        "best_reference": result["best_reference"],
        "health_scores": result["label_scores"],
    }
    if as_json:
        print(json.dumps(payload))
        return

    print(f"Species: {payload['species']}")
    print(f"Health: {payload['health']} ({float(payload['confidence']):.3f})")
    print(f"Best reference: {payload['best_reference']}")
    print(f"Scores: {payload['health_scores']}")


def cmd_predict_health_image(args: argparse.Namespace) -> int:
    refs = load_species_health_references(Path(args.refs), args.species)
    query = cv2.imread(args.image)
    if query is None:
        print("ERROR: Could not read query image")
        return 1
    result = predict(query, refs)
    print_health_result(args.species, result, args.json)
    return 0


def cmd_predict_health_camera(args: argparse.Namespace) -> int:
    refs = load_species_health_references(Path(args.refs), args.species)
    query = read_camera_frame(args.camera)
    result = predict(query, refs)
    print_health_result(args.species, result, args.json)
    return 0


def list_species(ref_root: Path) -> List[str]:
    if not ref_root.exists():
        return []
    return sorted([p.name for p in ref_root.iterdir() if p.is_dir()])


def prompt_yes_no(prompt: str, default_yes: bool = True) -> bool:
    suffix = "[Y/n]" if default_yes else "[y/N]"
    raw = input(f"{prompt} {suffix}: ").strip().lower()
    if not raw:
        return default_yes
    return raw in ("y", "yes")


def cmd_check_health(args: argparse.Namespace) -> int:
    ref_root = Path(args.refs)
    choices = list_species(ref_root)
    if not choices:
        print(f"ERROR: No species folders found in {ref_root}")
        return 1

    species = args.species
    if not species:
        print("Available species:")
        for idx, name in enumerate(choices, start=1):
            print(f"  {idx}. {name}")
        selected = input("Select species by name (or number): ").strip()
        if selected.isdigit() and 1 <= int(selected) <= len(choices):
            species = choices[int(selected) - 1]
        else:
            species = selected

    if species not in choices:
        print(f"ERROR: Unknown species '{species}'. Available: {', '.join(choices)}")
        return 1

    use_camera = prompt_yes_no("Use camera for health check?", default_yes=True)
    refs = load_species_health_references(ref_root, species)

    if use_camera:
        camera_idx = args.camera
        raw_cam = input(f"Camera index [{camera_idx}]: ").strip()
        if raw_cam:
            camera_idx = int(raw_cam)
        query = read_camera_frame(camera_idx)
    else:
        image_path = input("Enter query image path: ").strip()
        query = cv2.imread(image_path)
        if query is None:
            print("ERROR: Could not read query image")
            return 1

    result = predict(query, refs)
    print_health_result(species, result, as_json=False)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="No-training plant matcher")
    sub = parser.add_subparsers(dest="command", required=True)

    p_img = sub.add_parser("predict-image", help="Predict from a single image file")
    p_img.add_argument("--image", required=True, help="Path to query image")
    p_img.add_argument("--refs", default="vision/reference", help="Reference images root")
    p_img.add_argument("--labels", default=",".join(DEFAULT_LABELS), help="Comma-separated label list")
    p_img.add_argument("--json", action="store_true", help="Output JSON")
    p_img.set_defaults(func=cmd_predict_image)

    p_cam = sub.add_parser("predict-camera", help="Predict from one camera frame")
    p_cam.add_argument("--camera", type=int, default=0, help="Camera index (MacBook usually 0)")
    p_cam.add_argument("--refs", default="vision/reference", help="Reference images root")
    p_cam.add_argument("--labels", default=",".join(DEFAULT_LABELS), help="Comma-separated label list")
    p_cam.add_argument("--json", action="store_true", help="Output JSON")
    p_cam.set_defaults(func=cmd_predict_camera)

    p_himg = sub.add_parser("predict-health-image", help="Classify healthy/unhealthy from image for one species")
    p_himg.add_argument("--species", required=True, help="Species folder under reference root (e.g., aloe)")
    p_himg.add_argument("--image", required=True, help="Path to query image")
    p_himg.add_argument("--refs", default="vision/reference", help="Reference images root")
    p_himg.add_argument("--json", action="store_true", help="Output JSON")
    p_himg.set_defaults(func=cmd_predict_health_image)

    p_hcam = sub.add_parser("predict-health-camera", help="Classify healthy/unhealthy from camera for one species")
    p_hcam.add_argument("--species", required=True, help="Species folder under reference root (e.g., pothos)")
    p_hcam.add_argument("--camera", type=int, default=0, help="Camera index (MacBook usually 0)")
    p_hcam.add_argument("--refs", default="vision/reference", help="Reference images root")
    p_hcam.add_argument("--json", action="store_true", help="Output JSON")
    p_hcam.set_defaults(func=cmd_predict_health_camera)

    p_check = sub.add_parser("check-health", help="Interactive health check with camera/image selection")
    p_check.add_argument("--species", help="Optional species; if omitted you'll be prompted")
    p_check.add_argument("--camera", type=int, default=0, help="Default camera index")
    p_check.add_argument("--refs", default="vision/reference", help="Reference images root")
    p_check.set_defaults(func=cmd_check_health)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return int(args.func(args))
    except Exception as exc:
        print(f"ERROR: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
