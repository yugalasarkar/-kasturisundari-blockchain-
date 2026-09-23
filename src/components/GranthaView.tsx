import React, { useState, useMemo } from 'react';
import { Grantha, Khanda, CanonTier } from '../types';
import { Layers, ArrowRight, Compass, Search } from 'lucide-react';

interface GranthaViewProps {
  grantha: Grantha;
  availableGranthas: Grantha[];
  onSelectGrantha: (g: Grantha) => void;
  onSelectKhanda: (k: Khanda) => void;
  activeTierInfo?: CanonTier;
}

export const GranthaView: React.FC<GranthaViewProps> = ({
  grantha,
  availableGranthas,
  onSelectGrantha,
  onSelectKhanda,
  activeTierInfo
}) => {
  const [filterText, setFilterText] = useState('');

  const filteredKhandas = useMemo(() => {
    if (!filterText.trim()) return grantha.khandas;
    const term = filterText.toLowerCase();
    return grantha.khandas.filter(
      (k) =>
        k.name.toLowerCase().includes(term) ||
        (k.description && k.description.toLowerCase().includes(term))
    );
  }, [grantha.khandas, filterText]);

  return (
    <div className="space-y-8 animate-fadeIn">
      {/* १. क्षैतिज-ग्रन्थचयनम् (Horizontal Grantha Selector) */}
      {availableGranthas.length > 1 && (
        <div className="flex items-center gap-2 p-1.5 bg-[#12131a] rounded-lg border border-[#21232d] overflow-x-auto no-scrollbar scroll-smooth">
          <div className="text-xs font-serif text-stone-300 px-2.5 flex items-center gap-1.5 shrink-0 border-r border-[#21232d] pr-3">
            <Compass className="w-3.5 h-3.5 text-[#c5a059]" />
            <span>ग्रन्थचयनम् :</span>
          </div>
          <div className="flex items-center gap-1.5 shrink-0">
            {availableGranthas.map((g) => {
              const isSelected = g.id === grantha.id;
              return (
                <button
                  key={g.id}
                  id={`grantha-select-${g.id}`}
                  onClick={() => onSelectGrantha(g)}
                  className={`px-3.5 py-1.5 rounded-md text-xs transition-all font-serif cursor-pointer whitespace-nowrap flex items-center gap-1.5 ${
                    isSelected
                      ? 'bg-[#c5a059]/15 text-[#e8be6b] font-semibold border border-[#c5a059]/40 shadow-sm'
                      : 'text-stone-300 hover:text-stone-100 hover:bg-[#1b1d27] border border-transparent'
                  }`}
                >
                  <span className="font-serif text-[13px]">{g.title}</span>
                </button>
              );
            })}
          </div>
        </div>
      )}

      {/* २. मुख्य-ग्रन्थपरिचय-फलकम् (Dominant Devanagari Manuscript Card) */}
      <div 
        id="grantha-hero-card"
        className="p-6 sm:p-8 rounded-xl bg-gradient-to-b from-[#13141b] via-[#101117] to-[#0e0f14] border border-[#23242c] flex flex-col md:flex-row items-start md:items-center justify-between gap-6 shadow-xl relative overflow-hidden"
      >
        {/* Subtle geometry background */}
        <div className="absolute -right-16 -top-16 w-64 h-64 border border-[#c5a059]/5 rounded-full pointer-events-none" />
        <div className="absolute -right-8 -top-8 w-48 h-48 border border-[#c5a059]/10 rounded-full pointer-events-none" />

        <div className="space-y-3 relative z-10 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2">
            <span className="text-xs font-serif text-[#c5a059] bg-[#c5a059]/10 border border-[#c5a059]/20 px-2.5 py-0.5 rounded">
              {grantha.tag}
            </span>
            {activeTierInfo && (
              <span className="text-xs font-serif text-stone-300 bg-[#191b24] px-2.5 py-0.5 rounded border border-[#262834]">
                {activeTierInfo.label} • {activeTierInfo.sub}
              </span>
            )}
          </div>

          <h1 className="text-3xl sm:text-4xl font-bold text-stone-100 font-serif tracking-wide leading-tight">
            {grantha.title}
          </h1>

          {/* Sanskrit Mangalacharana / Mahatmya */}
          <p className="text-sm sm:text-base text-stone-200 font-serif leading-relaxed border-l-2 border-[#c5a059]/60 pl-3 py-1 whitespace-pre-line">
            « {grantha.desc} »
          </p>

          <div className="flex items-center gap-3 text-xs font-serif pt-1">
            <span className="text-[#e8be6b] font-serif text-sm">{grantha.totalVerses}</span>
            <span className="text-stone-500">•</span>
            <span className="text-stone-300 font-serif">{grantha.khandas.length} खण्डाः</span>
          </div>
        </div>

        <div className="w-full md:w-auto flex md:flex-col items-center justify-center p-4 rounded-xl bg-[#171924]/70 border border-[#252836] text-center gap-2 shrink-0">
          <div className="w-12 h-12 rounded-full bg-[#c5a059]/10 border border-[#c5a059]/30 flex items-center justify-center text-[#c5a059] font-serif text-2xl font-bold shadow-sm">
            ॐ
          </div>
          <div>
            <div className="text-xs font-semibold text-stone-200 font-serif">सनातन परम्परा</div>
            <div className="text-[11px] text-stone-400 font-serif">मूलग्रन्थपाठः</div>
          </div>
        </div>
      </div>

      {/* ३. खण्डानां पटलम् (Khandas Grid) */}
      <div className="space-y-4">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
          <h2 className="text-xs font-serif text-stone-300 uppercase tracking-widest flex items-center gap-2">
            <Layers className="w-4 h-4 text-[#c5a059]" />
            <span className="font-semibold text-stone-200">
              {grantha.id === 'padma-purana' ? 'सप्त खण्डाः' : 'ग्रन्थखण्डाः'}
            </span>
            <span className="text-stone-400 font-serif text-xs normal-case">
              ({grantha.khandas.length} खण्डाः)
            </span>
          </h2>

          {/* Quick filter input */}
          <div className="relative w-full sm:w-64">
            <Search className="w-3.5 h-3.5 absolute left-3 top-1/2 -translate-y-1/2 text-stone-500" />
            <input
              id="khanda-search-input"
              type="text"
              placeholder="खण्डान्वेषणम्..."
              value={filterText}
              onChange={(e) => setFilterText(e.target.value)}
              className="w-full pl-8 pr-3 py-1.5 text-xs bg-[#111218] border border-[#21232d] focus:border-[#c5a059]/60 rounded-lg text-stone-200 placeholder-stone-500 outline-none transition-colors font-serif"
            />
          </div>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-3.5">
          {filteredKhandas.map((k) => (
            <div
              key={k.id}
              id={`khanda-card-${k.id}`}
              onClick={() => onSelectKhanda(k)}
              className="group p-4 rounded-xl bg-[#12131a] border border-[#21232d] hover:border-[#c5a059]/70 hover:bg-[#161822] cursor-pointer transition-all duration-150 flex flex-col justify-between gap-3 shadow-sm hover:shadow-md select-none"
            >
              <div className="flex items-start justify-between gap-3">
                <div className="space-y-1">
                  <div className="flex items-center gap-2.5">
                    <span className="w-7 h-7 rounded-lg bg-[#1c1e28] border border-[#282a36] text-xs font-serif font-bold flex items-center justify-center text-[#c5a059] group-hover:border-[#c5a059]/50 group-hover:text-[#e8be6b] transition-colors">
                      {k.id}
                    </span>
                    <div className="font-bold text-lg text-stone-100 group-hover:text-[#e8be6b] font-serif transition-colors">
                      {k.name}
                    </div>
                  </div>
                </div>

                <div className="text-right shrink-0">
                  <div className="text-xs font-serif font-medium text-stone-200 bg-[#1a1c26] px-2.5 py-0.5 rounded border border-[#252834]">
                    {k.totalAdhyayas} अध्यायाः
                  </div>
                </div>
              </div>

              {k.description && (
                <p className="text-xs text-stone-300 pl-9 line-clamp-2 leading-relaxed font-serif">
                  {k.description}
                </p>
              )}

              {/* Clean confident प्रवेशः → link */}
              <div className="pt-2 border-t border-[#1d1f28] flex items-center justify-between text-xs pl-9">
                <span className="text-stone-400 font-serif text-[11px]">
                  {k.id === 7 && grantha.id === 'padma-purana' ? '★ सप्तमखण्डः' : `खण्डः ${k.id}`}
                </span>
                <span className="text-[#c5a059] font-medium opacity-90 group-hover:opacity-100 group-hover:text-[#e8be6b] transition-all flex items-center gap-1 font-serif text-xs">
                  प्रवेशः <ArrowRight className="w-3.5 h-3.5 group-hover:translate-x-1 transition-transform" />
                </span>
              </div>
            </div>
          ))}
        </div>

        {filteredKhandas.length === 0 && (
          <div className="p-8 text-center text-xs text-stone-400 bg-[#12131a] rounded-lg border border-[#21232d] font-serif">
            किमपि खण्डं न लब्धम्।
          </div>
        )}
      </div>
    </div>
  );
};
