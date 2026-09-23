import React from "react";
import dossier from "../../data/khandana/william-jones.json";

export const WilliamJonesDossier: React.FC = () => {
  const meta = dossier.target_metadata;
  const linguistic = dossier.linguistic_affirmation;

  return (
    <section className="mx-auto max-w-5xl px-4 py-12 text-neutral-100 font-sans">
      {/* Target Header */}
      <div className="rounded-2xl border border-neutral-800 bg-neutral-950 p-6 md:p-8 mb-10 shadow-2xl">
        <div className="flex flex-wrap items-center justify-between gap-4 border-b border-neutral-800 pb-6">
          <div>
            <span className="text-xs font-mono uppercase tracking-widest text-red-500 font-bold">
              Primary Dossier • Target 04 (The Founding Architect)
            </span>
            <h1 className="text-3xl md:text-4xl font-black text-white mt-1">
              {meta.name}
            </h1>
            <p className="text-sm font-mono text-neutral-400 mt-1">
              {meta.title} ({meta.tenure})
            </p>
          </div>
          <span className="rounded border border-red-500/30 bg-red-950/40 px-3 py-1 text-xs font-mono uppercase tracking-wider text-red-300">
            {meta.operational_role}
          </span>
        </div>

        {/* Epistemic Alert */}
        <div className="mt-6 rounded-xl border border-amber-500/30 bg-amber-950/20 p-5">
          <h2 className="text-sm font-bold uppercase tracking-wider text-amber-400 font-mono">
            {dossier.epistemic_warning.headline}
          </h2>
          <p className="mt-2 text-sm leading-relaxed text-neutral-300">
            {dossier.epistemic_warning.exposition}
          </p>
        </div>
      </div>

      {/* THE INDO-EUROPEAN LINGUISTIC REBUTTAL */}
      <div className="my-12 rounded-3xl border-2 border-blue-500 bg-[#030b1e] p-8 md:p-10 shadow-[0_0_50px_rgba(37,99,235,0.3)] relative overflow-hidden">
        <div className="border-b border-blue-900/60 pb-4">
          <span className="inline-block rounded-full border border-blue-400/50 bg-blue-950/80 px-4 py-1 text-xs font-mono uppercase tracking-widest text-blue-300">
            Philological Demolition • The Root of Speech
          </span>
          <h2 className="mt-3 text-2xl md:text-3xl font-black text-white">
            {linguistic.title}
          </h2>
          <p className="mt-1 text-xs md:text-sm font-mono text-blue-200/70">
            {linguistic.subtitle}
          </p>
        </div>

        <div className="mt-6 space-y-4 text-sm md:text-base leading-relaxed text-neutral-200">
          <div className="rounded-xl border border-blue-500/20 bg-blue-950/40 p-5">
            <h3 className="text-sm font-bold uppercase tracking-wider text-[#FFD700] font-mono mb-1">
              The Trojan Horse of Flattery
            </h3>
            <p className="text-neutral-300 text-xs md:text-sm">
              {linguistic.colonial_fabrication}
            </p>
          </div>

          <div className="rounded-xl border border-blue-500/20 bg-blue-950/40 p-5">
            <h3 className="text-sm font-bold uppercase tracking-wider text-[#FFD700] font-mono mb-1">
              The Sovereign Reality of Sanskrit
            </h3>
            <p className="text-neutral-300 text-xs md:text-sm font-semibold text-white">
              {linguistic.categorical_verdict}
            </p>
          </div>
        </div>
      </div>

      {/* FORENSIC INDICTMENTS LIST */}
      <div className="space-y-8 my-12">
        <div className="border-b border-neutral-800 pb-3">
          <span className="text-xs font-mono uppercase tracking-widest text-red-400 block mb-1">
            Forensic Indictment Catalog
          </span>
          <h3 className="text-xl md:text-2xl font-black text-white font-mono">
            Documented Scandals, Chronological Theft & Judicial Sabotage
          </h3>
        </div>

        {dossier.systemic_fabrications_and_scandals.map((entry) => (
          <article
            key={entry.id}
            className="rounded-xl border border-neutral-800 bg-neutral-900/80 p-6 md:p-8 transition-all hover:border-amber-500/40 shadow-lg"
          >
            <div className="flex flex-wrap items-center justify-between gap-2 border-b border-neutral-800 pb-3">
              <span className="rounded bg-neutral-800 px-2.5 py-0.5 text-xs font-mono text-amber-400 uppercase font-bold">
                {entry.category}
              </span>
              <span className="text-xs text-neutral-500 font-mono">ID: {entry.id}</span>
            </div>

            <h4 className="mt-3 text-lg md:text-xl font-bold text-white">
              {entry.title}
            </h4>

            <div className="mt-4 space-y-2 text-sm leading-relaxed text-neutral-300">
              <p>
                <strong className="text-neutral-100 font-semibold">Historical Reality: </strong>
                {entry.historical_fact}
              </p>
              <p>
                <strong className="text-red-400 font-semibold">Colonial Sabotage: </strong>
                {entry.colonial_sabotage}
              </p>
            </div>

            {entry.archival_sources && (
              <div className="mt-5 border-t border-neutral-800/80 pt-3">
                <span className="text-[11px] font-mono uppercase tracking-wider text-amber-400/90 block mb-2">
                  Archival Holdings & Primary Texts:
                </span>
                <ul className="space-y-2">
                  {entry.archival_sources.map((src, idx) => (
                    <li
                      key={idx}
                      className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 rounded bg-neutral-950 p-2.5 border border-neutral-800 text-xs"
                    >
                      <span className="text-neutral-300 italic pr-2">{src.citation}</span>
                      <a
                        href={src.archive_url}
                        target="_blank"
                        rel="noopener noreferrer"
                        className="inline-flex items-center gap-1 shrink-0 px-2.5 py-1 rounded bg-amber-500/10 hover:bg-amber-500/20 text-amber-300 border border-amber-500/30 font-mono text-[11px] transition-colors"
                      >
                        <span>Verify Archive</span>
                        <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M10 6H6a2 2 0 00-2 2v10a2 2 0 002 2h10a2 2 0 002-2v-4M14 4h6m0 0v6m0-6L10 14" />
                        </svg>
                      </a>
                    </li>
                  ))}
                </ul>
              </div>
            )}
          </article>
        ))}
      </div>

      {/* DEFINITIVE VERDICT */}
      <div className="mt-16 rounded-2xl border-2 border-red-500/60 bg-neutral-950 p-8 shadow-2xl relative">
        <span className="text-xs uppercase tracking-widest text-red-400 font-mono block mb-2">
          Definitive Verdict • Epistemic Impeachment
        </span>
        <h2 className="text-2xl font-bold text-white mb-4">
          {dossier.definitive_indictment_verdict.title}
        </h2>
        <blockquote className="border-l-4 border-red-500 pl-6 text-base md:text-lg leading-relaxed text-neutral-200 italic font-serif">
          "{dossier.definitive_indictment_verdict.verdict_statement}"
        </blockquote>
      </div>
    </section>
  );
};

export default WilliamJonesDossier;
