"""Environment / situation detection using YAMNet (via audio_model)."""

from __future__ import annotations

from typing import Any, Dict, List, Optional

import audio_model


def analyze(audio_path: str) -> Optional[Dict[str, Any]]:
    """Run YAMNet on a single WAV file; returns one result dict or None."""
    results: List[Dict[str, Any]] = audio_model.identify_sounds([audio_path])
    if not results:
        return None
    row = results[0]
    return {
        "clip": row["clip"],
        "situation": row["situation"],
        "confidence": float(row["confidence"]),
    }
