import React from "react";
import dossier from "../../data/khandana/thomas-macaulay.json";

export const MacaulayCartelConvergence: React.FC = () => {
  const matrix = (dossier as any).cartel_convergence_matrix;
  if (!matrix) return null;

  return (
    <div className="my-12 rounded-2xl border border-neutral-800 bg-neutral-950 p-6 md:p-8">
      <div className="border-b border-neutral-800 pb-4 mb-6">
        <span className="text-xs font-mono uppercase tracking-widest text-amber-400 block mb-1">
          Historical Conclave • 1855 London Conclave
        </span>
        <h2 className="text-2xl font-bold text-white">
          {matrix.title}
        </h2>
        <p className="text-xs font-mono text-neutral-400 mt-1">
          {matrix.subtitle}
        </p>
      </div>

      {/* Node 1: The 1855 London Conclave */}
      <div className="rounded-xl border border-neutral-800 bg-neutral-900/60 p-6">
        <div className="flex items-center gap-2 mb-2">
          <span className="h-2 w-2 rounded-full bg-amber-400" />
          <h3 className="text-base font-bold uppercase tracking-wider text-amber-300 font-mono">
            1. {matrix.the_1855_london_summit.summit_title}
          </h3>
        </div>
        <p className="text-xs font-mono text-neutral-400 mb-3">
          {matrix.the_1855_london_summit.historical_context}
        </p>
        <div className="space-y-3 text-sm text-neutral-300 leading-relaxed">
          <p className="border-l-2 border-neutral-700 pl-3 italic text-neutral-200">
            "{matrix.the_1855_london_summit.the_archival_confession}"
          </p>
          <p>
            <strong className="text-neutral-100 font-semibold">The Operational Division of Labor: </strong>
            {matrix.the_1855_london_summit.operational_division_of_labor}
          </p>
        </div>

        {/* Archival Verification Button */}
        {matrix.the_1855_london_summit.archival_sources && (
          <div className="mt-4 border-t border-neutral-800/80 pt-3">
            <span className="text-[11px] font-mono uppercase tracking-wider text-amber-400/90 block mb-2">
              Primary Archival Holding:
            </span>
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 rounded bg-neutral-950 p-2.5 border border-neutral-800 text-xs">
              <span className="text-neutral-300 italic">
                {matrix.the_1855_london_summit.archival_sources[0].citation}
              </span>
              <a
                href={matrix.the_1855_london_summit.archival_sources[0].archive_url}
                target="_blank"
                rel="noopener noreferrer"
                className="inline-flex items-center gap-1 shrink-0 px-2.5 py-1 rounded bg-amber-500/10 hover:bg-amber-500/20 text-amber-300 border border-amber-500/30 transition-colors font-mono text-[11px]"
              >
                <span>Verify Archive</span>
                <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M10 6H6a2 2 0 00-2 2v10a2 2 0 002 2h10a2 2 0 002-2v-4M14 4h6m0 0v6m0-6L10 14" />
                </svg>
              </a>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default MacaulayCartelConvergence;
