"""Frequency-domain background noise heuristic (via audio_model.check_background)."""

from __future__ import annotations

from typing import Any, Dict

import audio_model


def analyze(audio_path: str) -> Dict[str, Any]:
    """Detect background noise using the FFT magnitude band used in audio_model."""
    return {
        "clip": audio_path,
        "background_noise_detected": audio_model.check_background(audio_path),
    }
