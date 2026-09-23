import React from 'react';
import { 
  Box, 
  Coins, 
  Flame, 
  Activity, 
  Cpu, 
  ScrollText, 
  ShieldCheck,
  Server
} from 'lucide-react';
import { NetworkMetrics } from '../types';

interface MetricsBarProps {
  metrics: NetworkMetrics | null;
  totalBlocks: number;
  totalTxs: number;
  onSelectBlock: (blockNumber: number) => void;
  onOpenManuscripts: () => void;
}

export const MetricsBar: React.FC<MetricsBarProps> = ({
  metrics,
  totalBlocks,
  totalTxs,
  onSelectBlock,
  onOpenManuscripts,
}) => {
  return (
    <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-3 mb-6">
      {/* 1. Block Height */}
      <div 
        onClick={() => metrics?.blockNumber && onSelectBlock(metrics.blockNumber)}
        className="bg-slate-900/60 hover:bg-slate-900/90 border border-slate-800/80 hover:border-amber-500/30 rounded-xl p-3.5 transition-all cursor-pointer group"
      >
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Block Height</span>
          <Box className="w-4 h-4 text-amber-400 group-hover:scale-110 transition-transform" />
        </div>
        <div className="flex items-baseline space-x-1.5">
          <span className="text-lg font-bold font-mono text-white group-hover:text-amber-300 transition-colors">
            #{metrics?.blockNumber ? metrics.blockNumber.toLocaleString() : '---'}
          </span>
        </div>
        <div className="mt-1 flex items-center text-[11px] text-emerald-400 space-x-1">
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-500"></span>
          <span>Latest Finalized</span>
        </div>
      </div>

      {/* 2. Chain ID & Spec */}
      <div className="bg-slate-900/60 border border-slate-800/80 rounded-xl p-3.5">
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Chain ID</span>
          <Cpu className="w-4 h-4 text-cyan-400" />
        </div>
        <div className="flex items-baseline space-x-2">
          <span className="text-lg font-bold font-mono text-white">
            {metrics?.chainId || 108108}
          </span>
          <span className="text-xs font-mono text-cyan-300">0x1a64c</span>
        </div>
        <div className="mt-1 text-[11px] text-slate-400 truncate">
          EVM Shanghai / POS
        </div>
      </div>

      {/* 3. Native Currency */}
      <div className="bg-slate-900/60 border border-slate-800/80 rounded-xl p-3.5">
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Currency</span>
          <Coins className="w-4 h-4 text-amber-400" />
        </div>
        <div className="flex items-baseline space-x-1.5">
          <span className="text-lg font-bold font-mono text-amber-300">
            KST
          </span>
          <span className="text-xs text-slate-400">(18 Decimals)</span>
        </div>
        <div className="mt-1 text-[11px] text-slate-400">
          Kasturi Native Token
        </div>
      </div>

      {/* 4. Gas Price */}
      <div className="bg-slate-900/60 border border-slate-800/80 rounded-xl p-3.5">
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Gas Price</span>
          <Flame className="w-4 h-4 text-rose-400" />
        </div>
        <div className="flex items-baseline space-x-1">
          <span className="text-lg font-bold font-mono text-white">
            {metrics?.gasPriceGwei || 1}
          </span>
          <span className="text-xs text-slate-400">Gwei</span>
        </div>
        <div className="mt-1 text-[11px] text-slate-400">
          Base: 0x3b9aca00 wei
        </div>
      </div>

      {/* 5. Latency & Host Node */}
      <div className="bg-slate-900/60 border border-slate-800/80 rounded-xl p-3.5">
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Node RPC</span>
          <Activity className="w-4 h-4 text-emerald-400" />
        </div>
        <div className="flex items-baseline space-x-1">
          <span className="text-lg font-bold font-mono text-emerald-300">
            {metrics?.latencyMs || 32}
          </span>
          <span className="text-xs text-slate-400">ms latency</span>
        </div>
        <div className="mt-1 text-[11px] text-slate-400 truncate flex items-center space-x-1">
          <Server className="w-3 h-3 text-slate-400 shrink-0" />
          <span className="truncate">52.72.236.75 EC2</span>
        </div>
      </div>

      {/* 6. Manuscript Storage Proofs */}
      <div 
        onClick={onOpenManuscripts}
        className="bg-slate-900/60 hover:bg-slate-900/90 border border-slate-800/80 hover:border-amber-500/30 rounded-xl p-3.5 transition-all cursor-pointer group"
      >
        <div className="flex items-center justify-between text-slate-400 mb-1.5">
          <span className="text-xs font-medium uppercase tracking-wider">Vedic Proofs</span>
          <ScrollText className="w-4 h-4 text-amber-400 group-hover:scale-110 transition-transform" />
        </div>
        <div className="flex items-baseline space-x-1">
          <span className="text-lg font-bold font-mono text-white group-hover:text-amber-300 transition-colors">
            6/6
          </span>
          <span className="text-xs text-slate-400">Anchored</span>
        </div>
        <div className="mt-1 text-[11px] text-amber-400/90 flex items-center space-x-1">
          <ShieldCheck className="w-3 h-3" />
          <span>kasturi_storage</span>
        </div>
      </div>
    </div>
  );
};
