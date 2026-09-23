import React from "react";
import dossier from "../../data/khandana/monier-williams.json";
import { CentralSecretExposition } from "./CentralSecretExposition";
import { SayanaExposition } from "./SayanaExposition";

export const MonierWilliamsDossier: React.FC = () => {
  return (
    <section className="mx-auto max-w-5xl px-4 py-12 text-neutral-100 font-sans">
      {/* Critical Epistemic Quarantine Directive • Placed at the Absolute Top */}
      <aside className="mb-10 font-sans">
        <div className="rounded-2xl border-2 border-red-500 bg-red-950/40 p-6 md:p-8 shadow-[0_0_50px_rgba(239,68,68,0.25)] backdrop-blur-md">
          <div className="flex items-center gap-3 border-b border-red-500/40 pb-4">
            <span className="flex h-3 w-3 rounded-full bg-red-500 animate-ping" />
            <span className="text-xs font-mono uppercase tracking-widest text-red-400 font-bold">
              Epistemic Quarantine Order • Mandatory Archival Directive
            </span>
          </div>

          <div className="mt-5 space-y-4 text-neutral-100 text-sm md:text-base leading-relaxed">
            <p className="font-semibold text-red-200">
              For these documented reasons, the Oxford Sanskrit-English Dictionary and its derivative Sanskrit-Hindi translations must be completely abandoned. No academic claim, translation, or critical edition authored by Sir Monier Monier-Williams or Friedrich Max Müller can be accepted as authoritative. Western colonial translations of the Vedas and Purāṇas represent deliberate semantic distortions.
            </p>
            <p className="text-neutral-300">
              Authentic comprehension belongs exclusively to genuine living lineages (<span className="text-[#FFD700] font-bold">Paramparā</span>) and traditional indigenous Pandits grounded in the grammatical science of Pāṇini (<span className="text-blue-300 font-mono">Aṣṭādhyāyī</span>) and Yāska's <span className="text-blue-300 font-mono">Nirukta</span>. In light of these unearthed state papers, internal confessions, and institutional frauds, relying on colonial lexicons to interpret sacred texts is an indefensible surrender to imperial sabotage.
            </p>
          </div>
        </div>
      </aside>

      {/* Target Identity Header */}
      <div className="rounded-2xl border border-neutral-800 bg-neutral-950 p-6 md:p-8 mb-10 shadow-2xl">
        <div className="flex flex-wrap items-center justify-between gap-4 border-b border-neutral-800 pb-6">
          <div>
            <span className="text-xs font-mono uppercase tracking-widest text-red-400">
              Primary Dossier • Target 02
            </span>
            <h1 className="text-3xl md:text-4xl font-black text-white mt-1">
              {dossier.target_metadata.name}
            </h1>
            <p className="text-sm font-mono text-neutral-400 mt-1">
              {dossier.target_metadata.title} ({dossier.target_metadata.tenure})
            </p>
          </div>
          <span className="rounded border border-red-500/30 bg-red-950/40 px-3 py-1 text-xs font-mono uppercase tracking-wider text-red-300">
            {dossier.target_metadata.operational_role}
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

      {/* CORE SECRET REVELATION - PLACED CENTRALLY IN HIGH-IMPACT BLUE */}
      <CentralSecretExposition />

      {/* ĀCĀRYA SĀYAṆA VINDICATION - INDIGO BLUE & RADIANT GOLD */}
      <SayanaExposition />

      {/* Müller vs. Williams Reciprocal Exposure Section */}
      <div className="mb-12 rounded-2xl border border-neutral-800 bg-neutral-900/70 p-6 md:p-8">
        <div className="border-b border-neutral-800 pb-4 mb-6">
          <span className="text-xs font-mono uppercase tracking-widest text-red-400">
            Internal Colonial Feuds & Mutual Impeachment
          </span>
          <h2 className="text-2xl font-bold text-white mt-1">
            {dossier.muller_vs_williams_feud.section_title}
          </h2>
          <p className="text-sm text-neutral-400 mt-1">
            {dossier.muller_vs_williams_feud.subtitle}
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          {dossier.muller_vs_williams_feud.indictments.map((item) => (
            <div
              key={item.id}
              className="rounded-xl border border-neutral-800 bg-neutral-950 p-5 flex flex-col justify-between"
            >
              <div>
                <h3 className="text-base font-bold text-amber-300 mb-2">
                  {item.title}
                </h3>
                <p className="text-xs leading-relaxed text-neutral-300">
                  <strong className="text-neutral-100 font-semibold">Müller's Archival Disclosure: </strong>
                  {item.mullers_exposure}
                </p>
              </div>
              <p className="mt-4 pt-3 border-t border-neutral-800/80 text-[11px] text-neutral-400 italic">
                <strong className="text-amber-400/90 font-semibold">Critical Indictment: </strong>
                {item.colonial_reality}
              </p>
            </div>
          ))}
        </div>
      </div>

      {/* Main Indictment Cards */}
      <div className="space-y-8">
        {dossier.systemic_fabrications_and_scandals.map((entry) => (
          <article
            key={entry.id}
            className="rounded-xl border border-neutral-800 bg-neutral-900/90 p-6 md:p-8 transition-all hover:border-amber-500/40"
          >
            <div className="flex flex-wrap items-center justify-between gap-2 border-b border-neutral-800 pb-4">
              <span className="rounded bg-neutral-800 px-2.5 py-1 text-xs font-mono tracking-wider text-amber-400 uppercase">
                {entry.category}
              </span>
              <span className="text-xs text-neutral-500 font-mono">ID: {entry.id}</span>
            </div>

            <h3 className="mt-4 text-xl font-bold text-white tracking-wide">
              {entry.title}
            </h3>

            <div className="mt-4 space-y-3 text-sm leading-relaxed text-neutral-300">
              <p>
                <strong className="text-neutral-100 font-semibold">Historical Reality: </strong>
                {entry.historical_fact}
              </p>
              <p>
                <strong className="text-red-400 font-semibold">Imperial Execution: </strong>
                {entry.colonial_sabotage}
              </p>
            </div>

            {/* Archival Sources Verification */}
            {entry.archival_sources && (
              <div className="mt-6 border-t border-neutral-800/80 pt-4">
                <span className="text-[11px] font-mono uppercase tracking-wider text-amber-400/90 block mb-2">
                  Primary Archival Holdings & Verified Records:
                </span>
                <ul className="space-y-2">
                  {entry.archival_sources.map((source, idx) => (
                    <li
                      key={idx}
                      className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 rounded bg-neutral-950 p-2.5 border border-neutral-800/80 text-xs"
                    >
                      <span className="text-neutral-300 italic pr-2">
                        {source.citation}
                      </span>
                      <a
                        href={source.archive_url}
                        target="_blank"
                        rel="noopener noreferrer"
                        className="inline-flex items-center gap-1 shrink-0 px-2.5 py-1 rounded bg-amber-500/10 hover:bg-amber-500/20 text-amber-300 border border-amber-500/30 transition-colors font-mono text-[11px]"
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

      {/* Foundational Paratattva, Mahalakshmi & Svarupa-Shakti Affirmation */}
      <div className="my-16 rounded-2xl border-2 border-[#FFD700]/50 bg-neutral-950 p-8 md:p-10 text-neutral-100 shadow-[0_0_45px_rgba(255,215,0,0.18)] font-sans">
        <span className="text-xs font-mono uppercase tracking-widest text-[#FFD700] block mb-2">
          Ontological Sovereignty • The Transcendent Feminine Potency
        </span>
        <h2 className="text-2xl md:text-3xl font-bold text-white mb-4">
          Śrīmatī Rādhārāṇī & Mahālakṣmī: The Primordial Fountainhead Beyond Worldly Metrics
        </h2>
        
        <div className="space-y-4 text-sm md:text-base leading-relaxed text-neutral-300">
          <p>
            Colonial missionaries deployed a calculated double-assault against the Supreme Divine Feminine: they smeared <strong>Śrīmatī Rādhārāṇī</strong> on page 876 with Victorian moralistic vulgarity as a "mistress," while reducing <strong>Mahālakṣmī (Śrī)</strong> on page 892 and 1098 to crude, mercantile materialism—a counterfeit "Roman Fortuna" of fleeting worldly coins.
          </p>
          <p className="border-l-4 border-[#FFD700] pl-4 text-neutral-200 italic font-serif">
            "In the Vedic vision of the Śrī Sūkta, Mahālakṣmī is not an idol of temporal greed, but the luminous womb of cosmic consciousness, spiritual grace, and Mokṣa itself—inseparable from Ṭhākurāṇī Rādhikā as Her supreme majestic expansion. Words like 'Śrī' and 'Śraddhā' represent eternal states of ontological purity, not commercial transactions. Laws, colonial metrics, and dictionaries compiled for political domination are completely powerless before the Supreme Divine Couple. We reject their poisoned lexicons and declare our eternal shelter at Their lotus feet."
          </p>
        </div>

        <div className="mt-8 pt-6 border-t border-neutral-800 text-center">
          <p className="text-2xl md:text-3xl font-black tracking-widest text-[#FFD700] drop-shadow-[0_0_15px_rgba(255,215,0,0.6)] font-serif mb-1">
            राधे राधे
          </p>
          <p className="text-lg md:text-xl font-bold tracking-widest text-[#FFD700] drop-shadow-[0_0_20px_rgba(255,215,0,0.8)] font-serif">
            जय श्री राधिका कृष्ण • श्री महालक्ष्म्यै नमः
          </p>
        </div>
      </div>

      {/* Culminating Impeachment Verdict Banner */}
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

export default MonierWilliamsDossier;
