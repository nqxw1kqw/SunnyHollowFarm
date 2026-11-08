#!/usr/bin/env python3
"""
Procedural placeholder audio generator for Sunny Hollow Farm.
Synthesizes retro chiptune/pastoral sound effects and music loop.
All generated audio files are 100% public domain / CC0.
"""

import math
import os
import struct
import wave

SAMPLE_RATE = 44100

def write_wav(filename, samples, sample_rate=SAMPLE_RATE):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)
        
        raw_data = bytearray()
        for s in samples:
            # Clamp sample to [-1.0, 1.0]
            s = max(-1.0, min(1.0, s))
            int_val = int(s * 32767.0)
            raw_data.extend(struct.pack('<h', int_val))
        wav_file.writeframes(raw_data)
    print(f"Generated {filename} ({len(samples)} samples, {len(samples)/sample_rate:.2f}s)")

def sine_wave(freq, duration, volume=0.5, sample_rate=SAMPLE_RATE):
    n_samples = int(duration * sample_rate)
    samples = []
    for i in range(n_samples):
        t = i / sample_rate
        # Linear envelope to avoid clicking
        env = 1.0
        if i < 200:
            env = i / 200.0
        elif i > n_samples - 200:
            env = (n_samples - i) / 200.0
        samples.append(math.sin(2.0 * math.pi * freq * t) * volume * env)
    return samples

def noise(duration, volume=0.3, sample_rate=SAMPLE_RATE):
    n_samples = int(duration * sample_rate)
    samples = []
    # Simple deterministic LCG pseudo-random noise
    seed = 12345
    for i in range(n_samples):
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        val = (seed / 0x7fffffff) * 2.0 - 1.0
        env = 1.0 - (i / n_samples)
        samples.append(val * volume * env)
    return samples

def generate_till():
    # Earthy low thud with quick noise burst (0.16s)
    duration = 0.16
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    seed = 42
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        env = math.exp(-t * 28.0)
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        noise_val = ((seed / 0x7fffffff) * 2.0 - 1.0) * 0.4
        thud = math.sin(2.0 * math.pi * (120.0 - t * 350.0) * t) * 0.6
        samples.append((thud + noise_val) * env * 0.7)
    return samples

def generate_water():
    # Splashing trickle (0.28s)
    duration = 0.28
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        env = math.sin(math.pi * (i / n_samples))
        f1 = 450.0 + 200.0 * math.sin(2.0 * math.pi * 18.0 * t)
        f2 = 750.0 + 150.0 * math.cos(2.0 * math.pi * 24.0 * t)
        s = (math.sin(2.0 * math.pi * f1 * t) * 0.4 + math.sin(2.0 * math.pi * f2 * t) * 0.3)
        samples.append(s * env * 0.6)
    return samples

def generate_plant():
    # Soft organic pop/drop (0.12s)
    duration = 0.12
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        freq = 300.0 + 400.0 * (1.0 - (i / n_samples))
        env = math.sin(math.pi * (i / n_samples)) ** 0.5
        samples.append(math.sin(2.0 * math.pi * freq * t) * env * 0.5)
    return samples

def generate_harvest():
    # Cheerful upbeat chime (0.24s): rising major arpeggio note
    duration = 0.24
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        freq = 523.25 + (i / n_samples) * 523.25 # C5 to C6
        env = (1.0 - (i / n_samples)) ** 1.5
        s = math.sin(2.0 * math.pi * freq * t) + 0.3 * math.sin(4.0 * math.pi * freq * t)
        samples.append(s * env * 0.45)
    return samples

def generate_cook_success():
    # Triumphant 3-note jingle: C5 -> E5 -> G5 (0.36s)
    notes = [523.25, 659.25, 783.99]
    step_dur = 0.12
    samples = []
    for idx, f in enumerate(notes):
        n_samples = int(step_dur * SAMPLE_RATE)
        for i in range(n_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 6.0)
            s = math.sin(2.0 * math.pi * f * t) + 0.25 * math.sin(4.0 * math.pi * f * t)
            samples.append(s * env * 0.45)
    return samples

def generate_cook_fail():
    # Low descending buzz (0.22s)
    duration = 0.22
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        freq = 180.0 - 60.0 * (i / n_samples)
        env = 1.0 - (i / n_samples)
        s = 0.6 * math.sin(2.0 * math.pi * freq * t) + 0.4 * (1.0 if math.sin(2.0 * math.pi * freq * t) > 0 else -1.0)
        samples.append(s * env * 0.4)
    return samples

def generate_buy():
    # Two crisp rising coin tones (0.18s)
    step = 0.09
    notes = [987.77, 1318.51] # B5 -> E6
    samples = []
    for f in notes:
        n_samples = int(step * SAMPLE_RATE)
        for i in range(n_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 18.0)
            s = math.sin(2.0 * math.pi * f * t)
            samples.append(s * env * 0.45)
    return samples

def generate_sell():
    # Cash/coin register payout jingle (0.24s): E6 -> G#6 -> B6
    step = 0.08
    notes = [1318.51, 1661.22, 1975.53]
    samples = []
    for f in notes:
        n_samples = int(step * SAMPLE_RATE)
        for i in range(n_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 16.0)
            s = math.sin(2.0 * math.pi * f * t) + 0.2 * math.sin(4.0 * math.pi * f * t)
            samples.append(s * env * 0.4)
    return samples

def generate_sleep():
    # Gentle lullaby chime descending (0.45s): G5 -> E5 -> C5
    step = 0.15
    notes = [783.99, 659.25, 523.25]
    samples = []
    for f in notes:
        n_samples = int(step * SAMPLE_RATE)
        for i in range(n_samples):
            t = i / SAMPLE_RATE
            env = math.exp(-t * 7.0)
            s = math.sin(2.0 * math.pi * f * t) + 0.15 * math.sin(3.0 * math.pi * f * t)
            samples.append(s * env * 0.4)
    return samples

def generate_ui_open():
    # Subtle crisp swoosh/click (0.08s)
    duration = 0.08
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        freq = 350.0 + 450.0 * (i / n_samples)
        env = math.sin(math.pi * (i / n_samples))
        samples.append(math.sin(2.0 * math.pi * freq * t) * env * 0.35)
    return samples

def generate_ui_confirm():
    # Clean select blip (0.07s)
    duration = 0.07
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        freq = 880.0
        env = math.exp(-t * 35.0)
        samples.append(math.sin(2.0 * math.pi * freq * t) * env * 0.4)
    return samples

def generate_door():
    # Wooden door latch / creak (0.22s)
    duration = 0.22
    n_samples = int(duration * SAMPLE_RATE)
    samples = []
    seed = 888
    for i in range(n_samples):
        t = i / SAMPLE_RATE
        env = math.exp(-t * 12.0)
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        noise_val = ((seed / 0x7fffffff) * 2.0 - 1.0) * 0.35
        creak = math.sin(2.0 * math.pi * (160.0 + 80.0 * math.sin(2.0 * math.pi * 30.0 * t)) * t) * 0.5
        samples.append((creak + noise_val) * env * 0.5)
    return samples

def generate_music():
    # Gentle, pastoral 4-bar background music loop (4.8 seconds, seamless loop)
    # Chord progression: Cmaj -> Gmaj -> Amin -> Fmaj
    chords = [
        [261.63, 329.63, 392.00, 523.25], # C4, E4, G4, C5
        [196.00, 246.94, 293.66, 392.00], # G3, B3, D4, G4
        [220.00, 261.63, 329.63, 440.00], # A3, C4, E4, A4
        [174.61, 220.00, 261.63, 349.23], # F3, A3, C4, F4
    ]
    bar_dur = 1.2
    total_dur = bar_dur * len(chords)
    total_samples = int(total_dur * SAMPLE_RATE)
    mix = [0.0] * total_samples
    
    for bar_idx, chord in enumerate(chords):
        bar_start_sample = int(bar_idx * bar_dur * SAMPLE_RATE)
        # 4 eighth-note arpeggios per bar
        arp_dur = bar_dur / 4.0
        for note_idx in range(4):
            note_freq = chord[note_idx % len(chord)]
            note_start = bar_start_sample + int(note_idx * arp_dur * SAMPLE_RATE)
            note_samples = int(arp_dur * 1.5 * SAMPLE_RATE)
            for i in range(note_samples):
                dst = note_start + i
                if dst >= total_samples:
                    dst -= total_samples # wrap for seamless loop
                t = i / SAMPLE_RATE
                env = math.exp(-t * 3.5)
                # Soft flute/triangle-like tone
                s = (math.sin(2.0 * math.pi * note_freq * t) * 0.6 +
                     math.sin(4.0 * math.pi * note_freq * t) * 0.25 +
                     math.sin(6.0 * math.pi * note_freq * t) * 0.1) * env * 0.25
                mix[dst] += s

    # Normalize gently
    max_val = max(abs(x) for x in mix) or 1.0
    samples = [x / max_val * 0.45 for x in mix]
    return samples

def main():
    base_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets', 'audio')
    os.makedirs(base_dir, exist_ok=True)
    
    generators = {
        'music.wav': generate_music,
        'till.wav': generate_till,
        'water.wav': generate_water,
        'plant.wav': generate_plant,
        'harvest.wav': generate_harvest,
        'cook_success.wav': generate_cook_success,
        'cook_fail.wav': generate_cook_fail,
        'buy.wav': generate_buy,
        'sell.wav': generate_sell,
        'sleep.wav': generate_sleep,
        'ui_open.wav': generate_ui_open,
        'ui_confirm.wav': generate_ui_confirm,
        'door.wav': generate_door,
    }
    
    for filename, gen_fn in generators.items():
        out_path = os.path.join(base_dir, filename)
        samples = gen_fn()
        write_wav(out_path, samples)
    
    print("\nAll audio files generated successfully.")

if __name__ == '__main__':
    main()
