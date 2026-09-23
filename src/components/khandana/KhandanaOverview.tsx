import React from "react";

export const KhandanaOverview: React.FC = () => {
  return (
    <div className="mx-auto max-w-5xl px-4 py-8 font-sans text-neutral-100">
      {/* Top Epistemic Quarantine Warning */}
      <div className="mb-10 rounded-2xl border-2 border-red-500 bg-red-950/40 p-6 md:p-8 shadow-[0_0_50px_rgba(239,68,68,0.25)]">
        <div className="flex items-center gap-3 border-b border-red-500/40 pb-3">
          <span className="flex h-3 w-3 rounded-full bg-red-500 animate-ping" />
          <span className="text-xs font-mono uppercase tracking-widest text-red-400 font-bold">
            Mandatory Epistemic Quarantine Order
          </span>
        </div>
        <p className="mt-4 text-sm md:text-base leading-relaxed text-neutral-200">
          Western colonial translations of the Vedas, Upaniṣads, and Purāṇas represent deliberate instruments of geopolitical subjugation. Authentic comprehension belongs exclusively to living paramparā lineages, indigenous commentary tradition, and the grammatical science of Pāṇini (<span className="text-blue-300 font-mono">Aṣṭādhyāyī</span>).
        </p>
      </div>

      {/* Main Mission Box */}
      <div className="rounded-2xl border border-neutral-800 bg-neutral-950 p-8 shadow-2xl mb-12">
        <span className="text-xs font-mono uppercase tracking-widest text-amber-400 font-bold block mb-2">
          Philosophical Foundation
        </span>
        <h1 className="text-3xl md:text-4xl font-black text-white mb-4">
          What is Khandana (खण्डनम्)?
        </h1>
        <p className="text-base md:text-lg text-neutral-300 leading-relaxed">
          <strong className="text-white">Khandana (खण्डनम्)</strong> is the classical Sanskrit philosophical tradition of logical refutation, deconstruction, and forensic impeachment of flawed assertions using empirical evidence, epigraphy, and canonical grammatical science.
        </p>
        <p className="mt-4 text-sm md:text-base text-neutral-400 leading-relaxed">
          In this section, we unseal primary state papers, confidential correspondence, and internal administrative ledgers to expose the racial, economic, and missionary machinery behind 19th-century colonial Indology.
        </p>
      </div>

      {/* Forensic Pipeline: The Chain of Subversion */}
      <div className="my-12">
        <h2 className="text-xl font-bold uppercase tracking-wider text-neutral-200 font-mono mb-6">
          The Imperial Pipeline: A Synchronized Engine of Epistemicide
        </h2>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
          <div className="rounded-xl border border-neutral-800 bg-neutral-900/60 p-6">
            <span className="text-xs font-mono text-red-400 font-bold uppercase">Phase 01 • Enforcement</span>
            <h3 className="text-lg font-bold text-white mt-1">East India Co. & Macaulay</h3>
            <p className="mt-2 text-xs text-neutral-400 leading-relaxed">
              Fiscal looting, military linguistics, and legislative destruction of over 100,000 native Gurukulas to manufacture an alienated administrative class.
            </p>
          </div>

          <div className="rounded-xl border border-neutral-800 bg-neutral-900/60 p-6">
            <span className="text-xs font-mono text-amber-400 font-bold uppercase">Phase 02 • Semantic Subversion</span>
            <h3 className="text-lg font-bold text-white mt-1">Müller & Monier-Williams</h3>
            <p className="mt-2 text-xs text-neutral-400 leading-relaxed">
              Manufacturing poisoned lexicons (Oxford 1899), racial Aryan invasion dogmas, and the lexical desecration of Śrīmatī Rādhārāṇī and Mahālakṣmī.
            </p>
          </div>

          <div className="rounded-xl border border-neutral-800 bg-neutral-900/60 p-6">
            <span className="text-xs font-mono text-blue-400 font-bold uppercase">Phase 03 • Internal Colonization</span>
            <h3 className="text-lg font-bold text-white mt-1">Reformism to Gandhi</h3>
            <p className="mt-2 text-xs text-neutral-400 leading-relaxed">
              Implanting Protestant biblicism into indigenous reform sects, culminating in political leaders absorbing sacred texts through contaminated colonial translations.
            </p>
          </div>
        </div>
      </div>

      {/* Target Directory */}
      <div className="rounded-2xl border border-neutral-800 bg-neutral-950 p-6 md:p-8">
        <h3 className="text-lg font-bold text-white font-mono uppercase tracking-wider mb-2">
          Archival Dossier Navigation
        </h3>
        <p className="text-xs text-neutral-400 mb-6">
          Click any target tab in the navigation bar above to examine primary exhibits, unredacted letters, and full forensic autopsies:
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs font-mono">
          <div className="p-3 rounded-lg bg-neutral-900 border border-neutral-800">
            <span className="text-red-400 font-bold block">TARGET 01: FRIEDRICH MAX MÜLLER</span>
            <span className="text-neutral-400">EIC Paymaster contracts, Rigveda distortions, chronological fraud.</span>
          </div>
          <div className="p-3 rounded-lg bg-neutral-900 border border-neutral-800">
            <span className="text-red-400 font-bold block">TARGET 02: SIR MONIER MONIER-WILLIAMS</span>
            <span className="text-neutral-400">The 1899 Oxford Dictionary, Page 876 sacrilege, post-1857 cultural genocide.</span>
          </div>
          <div className="p-3 rounded-lg bg-neutral-900 border border-neutral-800">
            <span className="text-red-400 font-bold block">TARGET 03: THOMAS BABINGTON MACAULAY</span>
            <span className="text-neutral-400">The 1835 Education Minute, Gurukula eradication, UCL slave-compensation ties.</span>
          </div>
          <div className="p-3 rounded-lg bg-neutral-900 border border-neutral-800">
            <span className="text-amber-400 font-bold block">TARGET 04: THE EAST INDIA COMPANY</span>
            <span className="text-neutral-400">Fiscal looting, military surveillance, Fort William College operations.</span>
          </div>
        </div>
      </div>
    </div>
  );
};

export default KhandanaOverview;
