// Simple Web Audio API Tanpura Drone synthesizer for sacred recitation
let audioCtx: AudioContext | null = null;
let masterGain: GainNode | null = null;
let oscillators: OscillatorNode[] = [];
let isDronePlaying = false;

export function toggleDrone(onStateChange?: (playing: boolean) => void): boolean {
  if (isDronePlaying) {
    stopDrone();
    if (onStateChange) onStateChange(false);
    return false;
  } else {
    startDrone();
    if (onStateChange) onStateChange(true);
    return true;
  }
}

export function startDrone(): void {
  try {
    const AudioContextClass = window.AudioContext || (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
    if (!AudioContextClass) return;

    audioCtx = new AudioContextClass();
    masterGain = audioCtx.createGain();
    masterGain.gain.setValueAtTime(0.001, audioCtx.currentTime);
    masterGain.gain.exponentialRampToValueAtTime(0.08, audioCtx.currentTime + 2); // Soft ambient volume
    masterGain.connect(audioCtx.destination);

    // Tanpura Drone: C#3 fundamental (138.59 Hz), Pa (207.65 Hz), Sa' (277.18 Hz)
    const freqs = [138.59, 207.65, 277.18, 554.37];
    oscillators = freqs.map((freq, i) => {
      const osc = audioCtx!.createOscillator();
      const oscGain = audioCtx!.createGain();

      osc.type = i % 2 === 0 ? 'sine' : 'triangle';
      osc.frequency.setValueAtTime(freq, audioCtx!.currentTime);

      // Add gentle detune modulation for authentic tanpura shimmer
      osc.detune.setValueAtTime((i - 1.5) * 4, audioCtx!.currentTime);

      oscGain.gain.value = 0.25 / (i + 1);
      osc.connect(oscGain);
      oscGain.connect(masterGain!);

      osc.start();
      return osc;
    });

    isDronePlaying = true;
  } catch {
    isDronePlaying = false;
  }
}

export function stopDrone(): void {
  if (masterGain && audioCtx) {
    try {
      masterGain.gain.setValueAtTime(masterGain.gain.value, audioCtx.currentTime);
      masterGain.gain.exponentialRampToValueAtTime(0.0001, audioCtx.currentTime + 1);
      setTimeout(() => {
        oscillators.forEach(osc => {
          try { osc.stop(); osc.disconnect(); } catch { /* ignore */ }
        });
        oscillators = [];
        if (audioCtx && audioCtx.state !== 'closed') {
          audioCtx.close();
        }
        audioCtx = null;
        masterGain = null;
      }, 1000);
    } catch {
      // ignore cleanup errors
    }
  }
  isDronePlaying = false;
}
