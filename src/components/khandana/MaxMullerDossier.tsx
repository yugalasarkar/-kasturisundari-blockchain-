import React from 'react';
import dossierData from '../../data/khandana/max-muller.json';
import { BookOpen, ShieldAlert, FileText, AlertTriangle, Scale, Clock, Award } from 'lucide-react';

export const MaxMullerDossier: React.FC = () => {
  const cards = dossierData.dossier_cards || dossierData.dossier.quotes.map(q => ({
    card_id: q.id,
    badge: q.badge || `CRITICAL ANALYSIS / INDICTMENT`,
    date: q.date,
    recipient: q.recipient,
    primary_source: q.source,
    verbatim_quote: q.quote,
    analysis: q.analysis
  }));

  const critique = dossierData.structural_critique;
  const context = dossierData.dossier?.contextual_evidence;

  return (
    <section className="max-w-6xl mx-auto my-12 px-4 font-sans text-neutral-200 animate-fadeIn">
      {/* Module Title & Header Banner */}
      <div className="border-b border-red-800/80 pb-5 mb-8 flex flex-col md:flex-row md:items-center justify-between gap-4">
        <div>
          <div className="inline-flex items-center gap-2 px-3 py-1 rounded bg-red-950/80 border border-red-800 text-red-400 text-xs font-mono font-bold uppercase mb-2">
            <ShieldAlert className="w-4 h-4 text-red-500" />
            <span>{dossierData.header || "Khandana: Exposing Colonial Distortions"}</span>
          </div>
          <h2 className="text-3xl font-black tracking-wide text-red-500 uppercase font-serif">
            {dossierData.title || "Primary Evidence: Deconstructing Academic Neutrality"}
          </h2>
          <h3 className="text-lg text-amber-400/90 font-medium mt-1 font-serif">
            {dossierData.submodule || "Max Müller: The Manufactured Indologist & The Biblical Timeline Cover-up"}
          </h3>
        </div>
        <div className="bg-red-950/40 border border-red-900/60 px-4 py-3 rounded-xl flex items-center gap-3 shrink-0">
          <Scale className="w-7 h-7 text-red-400 shrink-0" />
          <div className="text-xs">
            <span className="text-red-300 font-bold block uppercase font-mono">PRIMARY ARCHIVAL DOSSIER</span>
            <span className="text-neutral-400">Friedrich Max Müller (1823–1900)</span>
          </div>
        </div>
      </div>

      {/* Opening Statement */}
      <div className="bg-neutral-900/90 border-l-4 border-red-600 border-y border-r border-neutral-800 p-6 mb-10 rounded-r-xl text-base sm:text-lg leading-relaxed shadow-2xl relative overflow-hidden">
        <div className="absolute right-0 top-0 bottom-0 w-2 bg-gradient-to-b from-red-600 via-amber-600 to-red-800"></div>
        <p className="text-neutral-200 font-serif leading-relaxed">
          {dossierData.introduction}
        </p>
        {dossierData.dossier?.summary && (
          <div className="mt-4 pt-4 border-t border-neutral-800 text-sm text-amber-300/90 font-mono flex items-start gap-2">
            <AlertTriangle className="w-4 h-4 text-amber-400 shrink-0 mt-0.5" />
            <span><strong>Dossier Summary:</strong> {dossierData.dossier.summary}</span>
          </div>
        )}
      </div>

      {/* Prominent Evidentiary Dossier Header Card */}
      <div className="mb-6 bg-gradient-to-r from-red-950/60 via-neutral-900 to-neutral-950 border-l-4 border-amber-500 border-y border-r border-neutral-800 p-5 rounded-r-xl shadow-xl flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <FileText className="w-6 h-6 text-amber-400" />
          <h3 className="text-xl font-bold font-serif text-amber-400 tracking-wide">
            DECONSTRUCTION: Primary Archival Admissions of Friedrich Max Müller
          </h3>
        </div>
        <span className="hidden sm:inline-flex bg-amber-500/10 border border-amber-500/30 text-amber-300 text-xs font-mono px-3 py-1 rounded-full">
          3 Direct Primary Sources
        </span>
      </div>

      {/* Primary Evidence Cards */}
      <div className="grid grid-cols-1 gap-8 mb-12">
        {cards.map((card, idx) => (
          <div 
            key={card.card_id || idx} 
            id={`dossier-card-${card.card_id || idx}`}
            className="bg-neutral-900/90 border border-neutral-800 rounded-xl p-6 shadow-2xl hover:border-red-900/80 transition-all relative overflow-hidden group"
          >
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 mb-4">
              <span className="inline-block bg-red-950/80 border border-red-800 text-red-400 text-xs font-mono font-bold px-3 py-1 rounded">
                {card.badge}
              </span>
              <span className="text-xs text-neutral-400 font-mono flex items-center gap-1">
                <Clock className="w-3.5 h-3.5 text-amber-400" />
                {card.date}
              </span>
            </div>

            <blockquote className="font-serif italic text-lg sm:text-xl text-neutral-100 border-l-4 border-amber-500/80 pl-4 py-2 my-4 bg-neutral-950/40 rounded-r">
              "{card.verbatim_quote}"
            </blockquote>

            <div className="text-xs text-neutral-400 font-mono mb-4 flex flex-wrap gap-4 bg-neutral-950 p-3 rounded-lg border border-neutral-800/80">
              <div><strong className="text-amber-400">Date:</strong> {card.date}</div>
              {card.recipient && <div><strong className="text-amber-400">Recipient:</strong> {card.recipient}</div>}
              <div><strong className="text-amber-400">Source:</strong> {card.primary_source}</div>
            </div>

            <div className="bg-neutral-950/90 border border-red-900/40 p-5 rounded-lg text-neutral-200 text-sm leading-relaxed shadow-inner">
              <div className="flex items-center space-x-2 mb-2">
                <Award className="w-4 h-4 text-red-400" />
                <strong className="text-red-400 font-mono uppercase text-xs tracking-wider">
                  CRITICAL ANALYSIS / INDICTMENT
                </strong>
              </div>
              <p className="text-neutral-300 font-serif leading-relaxed">
                {card.analysis}
              </p>
            </div>
          </div>
        ))}
      </div>

      {/* 6,000-Year Timeline Deep Dive & Comparative Theological Context Grid */}
      <div className="bg-neutral-900/80 border border-neutral-800 rounded-xl p-6 sm:p-8 shadow-2xl">
        <div className="border-b border-neutral-800 pb-4 mb-6">
          <h3 className="text-2xl font-bold font-serif text-amber-400 mb-1 flex items-center gap-2">
            <BookOpen className="w-6 h-6 text-amber-500" />
            <span>{critique?.title || context?.title || "The 6,000-Year Theological Boundary & Ussher Chronology"}</span>
          </h3>
          <p className="text-xs text-neutral-400 font-mono">
            Comparative analysis of 19th-century Anglican institutional boundaries versus indigenous multi-millennial Vedic astronomy.
          </p>
        </div>

        {critique?.sacred_antiquity && (
          <div className="bg-neutral-950 p-4 rounded-lg border border-amber-500/20 mb-6 text-sm text-neutral-300 font-serif leading-relaxed">
            <strong className="text-amber-400 font-mono uppercase text-xs block mb-1">Vedic Antiquity vs Colonial Reduction:</strong>
            {critique.sacred_antiquity}
          </div>
        )}

        <div className="grid md:grid-cols-3 gap-6 text-sm text-neutral-300">
          {/* Column 1: The Ecclesiastical Barrier / 4004 BCE Dogma */}
          <div className="bg-neutral-950 p-5 rounded-lg border border-neutral-800 flex flex-col justify-between">
            <div>
              <h4 className="font-bold text-amber-400 font-serif text-base mb-2 pb-2 border-b border-neutral-800 flex items-center gap-1.5">
                <Clock className="w-4 h-4 text-amber-400" />
                <span>1. The 4004 BCE Dogma</span>
              </h4>
              <p className="leading-relaxed text-xs text-neutral-300">
                {critique?.historical_proof_1_ussher?.details || context?.mechanism}
              </p>
            </div>
            
            <div className="mt-4 pt-3 border-t border-neutral-800 bg-neutral-900/60 p-3 rounded text-xs">
              <strong className="text-amber-400 block font-mono text-[10px] uppercase mb-1">Literary Anchor:</strong>
              <blockquote className="italic text-neutral-300 font-serif">
                "{critique?.historical_proof_3_shakespeare?.quote || context?.literary_anchor}"
              </blockquote>
              <span className="text-neutral-500 text-[10px] block mt-1">
                — {critique?.historical_proof_3_shakespeare?.source || "Shakespeare, As You Like It"}
              </span>
            </div>
          </div>

          {/* Column 2: Institutional Coercion & The Distortion Mechanism */}
          <div className="bg-neutral-950 p-5 rounded-lg border border-neutral-800 flex flex-col justify-between">
            <div>
              <h4 className="font-bold text-amber-400 font-serif text-base mb-2 pb-2 border-b border-neutral-800 flex items-center gap-1.5">
                <AlertTriangle className="w-4 h-4 text-amber-400" />
                <span>2. Institutional Coercion</span>
              </h4>
              <p className="leading-relaxed text-xs text-neutral-300">
                {critique?.historical_proof_2_distortion?.details || context?.institutional_coercion}
              </p>
            </div>
            <div className="mt-4 pt-3 border-t border-neutral-800 text-[11px] font-mono text-neutral-400">
              <span className="text-amber-400 font-bold">Funding Constraint:</span> Oxford & East India Company patronage enforced strict Anglican orthodoxy.
            </div>
          </div>

          {/* Column 3: The Final Indictment */}
          <div className="bg-neutral-950 p-5 rounded-lg border border-red-900/50 bg-red-950/10 flex flex-col justify-between">
            <div>
              <h4 className="font-bold text-red-400 font-serif text-base mb-2 pb-2 border-b border-red-900/50 flex items-center gap-1.5">
                <Scale className="w-4 h-4 text-red-400" />
                <span>3. The Final Indictment</span>
              </h4>
              <p className="leading-relaxed text-xs text-neutral-300 font-serif">
                {critique?.final_verdict || "Max Müller's assignment of 1200 BCE to the Vedas was an ideological surrender to Archbishop Ussher’s biblical 4004 BCE chronology to prevent contradiction with post-Flood biblical timelines."}
              </p>
            </div>
            <div className="mt-4 pt-3 border-t border-red-900/40 text-[11px] font-mono text-red-300">
              ✓ Deconstruction Complete: Colonial timelines proven driven by imperial dogma.
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
