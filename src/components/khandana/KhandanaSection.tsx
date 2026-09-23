import React, { useState } from 'react';
import { ShieldAlert, Sparkles } from 'lucide-react';
import { motion, AnimatePresence } from 'motion/react';
import { KhandanaOverview } from './KhandanaOverview';
import MaxMullerDossier from './MaxMullerDossier';
import MonierWilliamsDossier from './MonierWilliamsDossier';
import ThomasMacaulayDossier from './ThomasMacaulayDossier';
import RalphGriffithDossier from './RalphGriffithDossier';
import WilliamJonesDossier from './WilliamJonesDossier';

export const KhandanaSection: React.FC = () => {
  const [activeTab, setActiveTab] = useState<"OVERVIEW" | "MAX MÜLLER" | "MONIER-WILLIAMS" | "THOMAS MACAULAY" | "RALPH GRIFFITH" | "WILLIAM JONES" | "EAST INDIA CO." | "BRAHMO / ARYA SAMAJ" | "M. K. GANDHI">("OVERVIEW");

  const tabs = [
    "OVERVIEW",
    "MAX MÜLLER",
    "MONIER-WILLIAMS",
    "THOMAS MACAULAY",
    "RALPH GRIFFITH",
    "WILLIAM JONES",
    "EAST INDIA CO.",
    "BRAHMO / ARYA SAMAJ",
    "M. K. GANDHI"
  ] as const;

  return (
    <div className="space-y-8 animate-fade-in max-w-5xl mx-auto">
      {/* Main Header / Banner matching Radha & Krishna sections */}
      <div className="relative rounded-none border-2 border-gold/40 overflow-hidden bg-[#080808] p-6 md:p-12 text-center shadow-2xl space-y-6">
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_center,rgba(212,175,55,0.12),transparent_70%)] pointer-events-none" />
        
        <ShieldAlert className="w-12 h-12 text-gold mx-auto animate-pulse" />
        
        <h2 className="text-3xl md:text-5xl font-serif tracking-tight font-black text-white uppercase">
          Khandana — Exposing Colonial Distortions
        </h2>
        
        <p className="text-zinc-300 text-sm md:text-base max-w-3xl mx-auto leading-relaxed font-sans">
          खण्डनम्: Primary archival evidence, philological deconstruction, and historical impeachment of colonial Indological fabrications.
        </p>

        <div className="inline-block border border-gold/50 px-6 py-2 bg-gold/5">
          <span className="text-gold font-mono text-xs tracking-[0.2em] uppercase font-bold">
            PRIMARY ARCHIVAL EVIDENCE & REBUTTALS
          </span>
        </div>
      </div>

      {/* Sub-navigation Tabs matching Radha.tsx */}
      <div className="flex flex-wrap justify-center border-b border-gold/20 mb-8 max-w-5xl mx-auto gap-1">
        {tabs.map((tab) => (
          <button
            key={tab}
            onClick={() => setActiveTab(tab)}
            className={`px-4 md:px-5 py-3 text-xs font-mono tracking-widest uppercase transition-all duration-300 ${
              activeTab === tab
                ? "text-gold border-b-2 border-gold font-black bg-gold/5"
                : "text-zinc-400 hover:text-white border-b-2 border-transparent hover:bg-white/5"
            }`}
          >
            {tab}
          </button>
        ))}
      </div>

      {/* Tab Content Area */}
      <AnimatePresence mode="wait">
        <motion.div
          key={activeTab}
          initial={{ opacity: 0, y: 15 }}
          animate={{ opacity: 1, y: 0 }}
          exit={{ opacity: 0, y: -15 }}
          transition={{ duration: 0.4 }}
        >
          {activeTab === "OVERVIEW" && (
            <KhandanaOverview />
          )}

          {activeTab === "MAX MÜLLER" && (
            <MaxMullerDossier />
          )}

          {activeTab === "MONIER-WILLIAMS" && (
            <MonierWilliamsDossier />
          )}

          {activeTab === "THOMAS MACAULAY" && (
            <ThomasMacaulayDossier />
          )}

          {activeTab === "RALPH GRIFFITH" && (
            <RalphGriffithDossier />
          )}

          {activeTab === "WILLIAM JONES" && (
            <WilliamJonesDossier />
          )}

          {activeTab !== "OVERVIEW" && activeTab !== "MAX MÜLLER" && activeTab !== "MONIER-WILLIAMS" && activeTab !== "THOMAS MACAULAY" && activeTab !== "RALPH GRIFFITH" && activeTab !== "WILLIAM JONES" && (
            <div className="bg-[#080808] border-2 border-gold/30 rounded-none p-8 md:p-12 text-center shadow-2xl space-y-4">
              <Sparkles className="w-8 h-8 text-gold mx-auto" />
              <h3 className="text-xl font-serif text-white uppercase tracking-widest font-black">
                {activeTab} DOSSIER
              </h3>
              <p className="text-zinc-400 font-mono text-xs max-w-xl mx-auto leading-relaxed">
                Primary archival research and correspondence index for {activeTab} is currently being transcribed and verified for absolute accuracy.
              </p>
              <div className="inline-block border border-gold/40 px-4 py-1.5 text-gold font-mono text-[10px] uppercase">
                COMING SOON IN KHANDANA ARCHIVE
              </div>
            </div>
          )}
        </motion.div>
      </AnimatePresence>
    </div>
  );
};

export default KhandanaSection;
