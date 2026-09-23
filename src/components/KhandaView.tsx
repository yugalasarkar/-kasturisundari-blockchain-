import React, { useState, useMemo } from 'react';
import { Khanda, Adhyaya, Grantha } from '../types';
import { ArrowLeft, ArrowRight, Search, BookOpen } from 'lucide-react';

interface KhandaViewProps {
  grantha: Grantha;
  khanda: Khanda;
  adhyayas: Adhyaya[];
  onSelectAdhyaya: (adhyayaId: number) => void;
  onBackToGrantha: () => void;
}

export const KhandaView: React.FC<KhandaViewProps> = ({
  grantha,
  khanda,
  adhyayas,
  onSelectAdhyaya,
  onBackToGrantha
}) => {
  const [searchQuery, setSearchQuery] = useState('');

  const filteredAdhyayas = useMemo(() => {
    if (!searchQuery.trim()) return adhyayas;
    const q = searchQuery.toLowerCase();
    return adhyayas.filter(
      (a) =>
        a.title.toLowerCase().includes(q) ||
        a.summary.toLowerCase().includes(q) ||
        a.id.toString() === q
    );
  }, [adhyayas, searchQuery]);

  return (
    <div className="space-y-6 animate-fadeIn">
      {/* Back button with clean Devanagari */}
      <button 
        id="back-to-grantha-btn"
        onClick={onBackToGrantha}
        className="text-xs text-stone-300 hover:text-[#c5a059] flex items-center gap-1.5 transition-colors cursor-pointer group py-1 font-serif"
      >
        <ArrowLeft className="w-3.5 h-3.5 group-hover:-translate-x-0.5 transition-transform text-[#c5a059]" /> 
        <span>प्रतिनिवर्तनम्</span>
        <span className="text-[#c5a059] font-serif text-xs">({grantha.title})</span>
      </button>

      {/* Khanda details banner */}
      <div 
        id="khanda-header-banner"
        className="p-6 sm:p-7 rounded-xl bg-gradient-to-r from-[#12131a] via-[#151722] to-[#101117] border border-[#23242c] flex flex-col sm:flex-row sm:items-center justify-between gap-4 shadow-lg"
      >
        <div className="space-y-1.5">
          <div className="flex items-center gap-2">
            <span className="text-xs font-serif text-[#c5a059] bg-[#c5a059]/10 px-2.5 py-0.5 rounded border border-[#c5a059]/20">
              खण्डः {khanda.id}
            </span>
            <span className="text-xs text-stone-300 font-serif">
              {grantha.title}
            </span>
          </div>
          <h1 className="text-2xl sm:text-3xl font-bold text-stone-100 font-serif">
            {khanda.name}
          </h1>
          <div className="flex items-center gap-3 pt-0.5">
            <span className="text-xs sm:text-sm font-serif text-[#e8be6b]">
              {khanda.totalAdhyayas} अध्यायाः
            </span>
          </div>
          {khanda.description && (
            <p className="text-xs text-stone-300 max-w-2xl pt-1 leading-relaxed font-serif">
              {khanda.description}
            </p>
          )}
        </div>

        {/* Total chapters badge */}
        <div className="flex sm:flex-col items-end justify-between sm:justify-center border-t sm:border-t-0 border-[#1f212c] pt-3 sm:pt-0 shrink-0">
          <div className="text-xs font-serif text-stone-300">
            सम्पूर्ण-अध्यायसूची
          </div>
          <div className="text-sm font-bold text-[#e8be6b] font-serif">
            {khanda.totalAdhyayas} अध्यायाः
          </div>
        </div>
      </div>

      {/* Search & chapter count */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
        <div className="text-xs font-serif text-stone-300 flex items-center gap-2">
          <BookOpen className="w-4 h-4 text-[#c5a059]" />
          <span className="font-semibold text-stone-200">अध्यायसूची</span>
          <span className="text-stone-400 font-serif text-xs">
            ({filteredAdhyayas.length} / {khanda.totalAdhyayas})
          </span>
        </div>

        <div className="relative w-full sm:w-72">
          <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-stone-500" />
          <input
            id="adhyaya-search-input"
            type="text"
            placeholder="अध्यायान्वेषणम्..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-8 pr-3 py-1.5 text-xs bg-[#111218] border border-[#21232d] focus:border-[#c5a059]/60 rounded-lg text-stone-200 placeholder-stone-500 outline-none transition-colors font-serif"
          />
        </div>
      </div>

      {/* Grid of Adhyāyas (Adhyāyas Grid) */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-3">
        {filteredAdhyayas.map((a) => (
          <div
            key={a.id}
            id={`adhyaya-card-${a.id}`}
            onClick={() => onSelectAdhyaya(a.id)}
            className="group p-4 rounded-lg bg-[#111218] border border-[#21232d] hover:border-[#c5a059]/70 hover:bg-[#171924] cursor-pointer transition-all flex flex-col justify-between min-h-[120px] shadow-sm hover:shadow-md select-none"
          >
            <div className="flex justify-between items-start gap-2">
              <span className="text-sm font-bold text-stone-200 font-serif group-hover:text-[#e8be6b] transition-colors">
                {a.title}
              </span>
              <span className="text-[10px] font-serif text-stone-300 bg-[#191b24] px-1.5 py-0.5 rounded border border-[#242633] shrink-0">
                {a.verseCount} श्लोकाः
              </span>
            </div>

            <div className="text-[11px] text-stone-300 font-serif line-clamp-2 my-2 leading-relaxed">
              {a.summary}
            </div>

            <div className="pt-2 border-t border-[#1a1c24] flex items-center justify-between text-xs font-serif">
              <span className="text-stone-400 font-serif text-[11px]">
                अध्यायः {a.id}
              </span>
              {/* Clean confident पठ्यताम् → */}
              <span className="text-[#c5a059] flex items-center gap-1 group-hover:text-[#e8be6b] group-hover:translate-x-0.5 transition-all font-medium text-xs">
                पठ्यताम् <ArrowRight className="w-3 h-3" />
              </span>
            </div>
          </div>
        ))}
      </div>

      {filteredAdhyayas.length === 0 && (
        <div className="p-12 text-center text-xs text-stone-400 bg-[#111218] rounded-lg border border-[#21232d] font-serif">
          किमपि अध्यायं न लब्धम्।
        </div>
      )}
    </div>
  );
};
