import React from "react";
import dossier from "../../data/khandana/monier-williams.json";

export const CentralSecretExposition: React.FC = () => {
  const secret = dossier.the_secret_core_indictment;

  return (
    <div className="my-16 mx-auto max-w-4xl px-4 font-sans text-center">
      {/* Central Blue Impeachment Box */}
      <div className="rounded-3xl border-2 border-blue-500 bg-neutral-950 p-8 md:p-12 shadow-[0_0_50px_rgba(59,130,246,0.3)] relative overflow-hidden">
        {/* Subtle Background Radial Accent */}
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_center,_var(--tw-gradient-stops))] from-blue-900/20 via-transparent to-transparent pointer-events-none" />

        {/* Category Badge */}
        <span className="inline-block rounded-full border border-blue-400/40 bg-blue-950/60 px-4 py-1 text-xs font-mono uppercase tracking-widest text-blue-400">
          Classified Missionary Archives • Core Strategic Exposure
        </span>

        {/* Main Title */}
        <h2 className="mt-6 text-2xl md:text-3xl font-black tracking-tight text-blue-400 leading-snug">
          {secret.section_title}
        </h2>
        <p className="mt-2 text-sm font-mono text-neutral-400">
          {secret.subtitle}
        </p>

        {/* The Citadel & The Obstacle */}
        <div className="mt-8 space-y-4 text-left text-sm md:text-base leading-relaxed text-neutral-200">
          <div className="rounded-2xl border border-blue-500/30 bg-blue-950/20 p-6">
            <h3 className="text-base font-bold text-blue-300 font-mono uppercase tracking-wide mb-2">
              1. The Impenetrable Citadel of Prema
            </h3>
            <p className="text-neutral-300">
              {secret.the_unbroken_citadel.finding}
            </p>
            <p className="mt-3 text-neutral-300">
              <strong className="text-blue-300 font-semibold">The Strategic Impasse: </strong>
              {secret.the_unbroken_citadel.the_obstacle}
            </p>
          </div>

          {/* Subversion Strategy */}
          <div className="rounded-2xl border border-blue-500/30 bg-blue-950/20 p-6">
            <h3 className="text-base font-bold text-blue-300 font-mono uppercase tracking-wide mb-2">
              2. Engineered Alienation & Lexical Defamation
            </h3>
            <p className="text-neutral-300">
              {secret.subversion_strategy.operational_directive}
            </p>
          </div>

          {/* Direct Archival Confessions from Peers */}
          <div className="pt-2">
            <h4 className="text-xs font-mono uppercase tracking-wider text-blue-400 text-center mb-4">
              Corroborating Archival Confessions from Inner-Circle Orientalists
            </h4>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              {secret.archival_confessions.map((c, idx) => (
                <div key={idx} className="rounded-xl border border-blue-500/20 bg-neutral-900/90 p-5">
                  <span className="text-xs font-bold text-blue-300 font-mono block">
                    {c.operative}
                  </span>
                  <span className="text-[11px] text-neutral-400 block mb-2">
                    {c.designation}
                  </span>
                  <blockquote className="text-xs text-neutral-300 italic border-l-2 border-blue-500/50 pl-3">
                    "{c.confession}"
                  </blockquote>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Epistemic Conclusion Callout */}
        <div className="mt-8 rounded-xl border border-red-500/40 bg-red-950/20 p-4 text-xs font-mono text-neutral-300">
          Monier-Williams and his apparatus systematically bypassed foundational Vedic references, isolating later Purāṇas to invent the falsehood that Śrīmatī Rādhārāṇī was a "late poetic fabrication"—a targeted subversion engineered to erode the very core of Vedic consciousness.
        </div>

        {/* Sacred Inscriptions in Radiant Pure Gold (English) */}
        <div className="mt-12 pt-8 border-t border-neutral-800 text-center">
          <p className="text-2xl md:text-3xl font-extrabold tracking-widest text-[#FFD700] drop-shadow-[0_0_15px_rgba(255,215,0,0.6)] font-serif mb-2">
            RADHE RADHE
          </p>
          <p className="text-xl md:text-2xl font-black tracking-widest text-[#FFD700] drop-shadow-[0_0_20px_rgba(255,215,0,0.8)] font-serif">
            JAI SHRI RADHIKA KRISHNA
          </p>
        </div>
      </div>
    </div>
  );
};

export default CentralSecretExposition;
