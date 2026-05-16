"""Estimate speech vs. background noise using DeepFilterNet enhancement residual."""

from __future__ import annotations

import threading
from typing import Any, Dict, Optional, Tuple

import numpy as np

_DF_LOCK = threading.Lock()
_DF_STATE: Optional[Tuple[Any, Any]] = None


def _model_and_state():
    global _DF_STATE
    if _DF_STATE is not None:
        return _DF_STATE
    from df.enhance import init_df

    model, df_state, _suffix = init_df(
        model_base_dir=None,
        post_filter=False,
        log_level="ERROR",
        log_file=None,
    )
    _DF_STATE = (model, df_state)
    return _DF_STATE


def analyze(audio_path: str) -> Dict[str, Any]:
    """
    Enhance audio with DeepFilterNet; treat enhanced energy as speech-dominated
    and (noisy - enhanced) as background-dominated residual.
    """
    try:
        import torch
        from df.enhance import enhance
        from df.io import load_audio
        from df.model import ModelParams
    except ImportError as e:
        return {"clip": audio_path, "error": f"DeepFilterNet stack unavailable: {e}"}

    try:
        with _DF_LOCK:
            model, df_state = _model_and_state()
            df_sr = ModelParams().sr
            noisy, _meta = load_audio(audio_path, sr=df_sr, verbose=False)
            noisy = noisy.float()
            enhanced = enhance(model, df_state, noisy, pad=True)
            enhanced = enhanced.float()

            min_len = min(noisy.shape[-1], enhanced.shape[-1])
            noisy = noisy[..., :min_len]
            enhanced = enhanced[..., :min_len]

            residual = noisy - enhanced
            speech_power = float(torch.mean(enhanced**2).item())
            noise_power = float(torch.mean(residual**2).item())
            denom = speech_power + noise_power + 1e-12
            speech_fraction = speech_power / denom
            ratio = speech_power / (noise_power + 1e-12)
            ratio_db = float(10.0 * np.log10(ratio + 1e-12))

        return {
            "clip": audio_path,
            "sample_rate_hz": int(df_sr),
            "speech_power": speech_power,
            "residual_noise_power": noise_power,
            "speech_fraction": speech_fraction,
            "speech_to_noise_power_ratio": ratio,
            "speech_to_noise_power_ratio_db": ratio_db,
        }
    except Exception as e:
        return {"clip": audio_path, "error": str(e)}
