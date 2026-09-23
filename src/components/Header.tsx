import React, { useState } from 'react';
import { 
  Search, 
  Layers, 
  ScrollText, 
  Server, 
  Radio, 
  Activity, 
  Zap, 
  Settings2, 
  RefreshCw,
  ExternalLink,
  ShieldCheck,
  CheckCircle2,
  AlertCircle
} from 'lucide-react';
import { NetworkMetrics } from '../types';

interface HeaderProps {
  metrics: NetworkMetrics | null;
  activeTab: 'explorer' | 'manuscripts' | 'devops';
  setActiveTab: (tab: 'explorer' | 'manuscripts' | 'devops') => void;
  onSearch: (query: string) => void;
  onRefresh: () => void;
  isRefreshing: boolean;
  rpcEndpoint: string;
  onOpenRpcSettings: () => void;
}

export const Header: React.FC<HeaderProps> = ({
  metrics,
  activeTab,
  setActiveTab,
  onSearch,
  onRefresh,
  isRefreshing,
  rpcEndpoint,
  onOpenRpcSettings,
}) => {
  const [searchQuery, setSearchQuery] = useState('');
  const [searchTypeHint, setSearchTypeHint] = useState<string | null>(null);

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = e.target.value.trim();
    setSearchQuery(e.target.value);

    if (!val) {
      setSearchTypeHint(null);
      return;
    }

    if (/^\d+$/.test(val)) {
      setSearchTypeHint('Block Number');
    } else if (/^0x[a-fA-F0-9]{40}$/.test(val)) {
      setSearchTypeHint('Account / Contract Address');
    } else if (/^0x[a-fA-F0-9]{64}$/.test(val)) {
      setSearchTypeHint('Transaction / Manuscript Hash');
    } else if (val.toLowerCase().startsWith('bafy') || val.toLowerCase().includes('manuscript') || val.toLowerCase().includes('veda')) {
      setSearchTypeHint('Kasturi Storage Manuscript');
    } else {
      setSearchTypeHint('Search text or query');
    }
  };

  const handleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (searchQuery.trim()) {
      onSearch(searchQuery.trim());
    }
  };

  return (
    <header className="border-b border-slate-800/80 bg-slate-950/90 backdrop-blur-md sticky top-0 z-40">
      {/* Top Banner / Network Status Bar */}
      <div className="bg-slate-900/80 border-b border-slate-800/60 px-4 py-1.5 text-xs text-slate-300">
        <div className="max-w-7xl mx-auto flex flex-wrap items-center justify-between gap-3">
          <div className="flex items-center space-x-4">
            <div className="flex items-center space-x-1.5">
              <span className="relative flex h-2 w-2">
                <span className={`animate-ping absolute inline-flex h-full w-full rounded-full ${metrics?.isLive ? 'bg-emerald-400 opacity-75' : 'bg-amber-400 opacity-75'}`}></span>
                <span className={`relative inline-flex rounded-full h-2 w-2 ${metrics?.isLive ? 'bg-emerald-500' : 'bg-amber-500'}`}></span>
              </span>
              <span className="font-medium text-slate-200">KasturiChain</span>
              <span className="text-slate-400">ID:</span>
              <span className="font-mono text-amber-400 font-semibold">{metrics?.chainId || 108108}</span>
              <span className="text-slate-400 font-mono text-[11px]">(0x1a64c)</span>
            </div>

            <div className="hidden sm:flex items-center space-x-1 text-slate-400">
              <span>Block:</span>
              <span className="font-mono text-slate-200 font-semibold">
                #{metrics?.blockNumber ? metrics.blockNumber.toLocaleString() : '---'}
              </span>
            </div>

            <div className="hidden md:flex items-center space-x-1 text-slate-400">
              <Zap className="w-3 h-3 text-amber-400" />
              <span>Gas:</span>
              <span className="font-mono text-slate-200 font-semibold">{metrics?.gasPriceGwei || 1} Gwei</span>
            </div>

            <div className="hidden lg:flex items-center space-x-1 text-slate-400">
              <Activity className="w-3 h-3 text-cyan-400" />
              <span>Latency:</span>
              <span className="font-mono text-slate-200 font-semibold">{metrics?.latencyMs || 28}ms</span>
            </div>
          </div>

          <div className="flex items-center space-x-3">
            <div className="flex items-center space-x-1 text-slate-400">
              <span className="text-[11px] hidden xl:inline">RPC:</span>
              <button 
                onClick={onOpenRpcSettings}
                className="font-mono text-[11px] text-slate-300 hover:text-amber-300 transition-colors flex items-center gap-1 bg-slate-800/80 px-2 py-0.5 rounded border border-slate-700/60"
                title="Change RPC Endpoint"
              >
                <Radio className="w-3 h-3 text-emerald-400" />
                <span className="truncate max-w-[140px] sm:max-w-[190px]">{rpcEndpoint.replace('https://', '')}</span>
                <Settings2 className="w-2.5 h-2.5 text-slate-400" />
              </button>
            </div>

            <button
              onClick={onRefresh}
              disabled={isRefreshing}
              className="p-1 rounded bg-slate-800/80 hover:bg-slate-700 text-slate-300 hover:text-white border border-slate-700/60 transition-all flex items-center gap-1 text-[11px] px-2"
              title="Refresh network state"
            >
              <RefreshCw className={`w-3 h-3 ${isRefreshing ? 'animate-spin text-amber-400' : ''}`} />
              <span className="hidden sm:inline">Refresh</span>
            </button>
          </div>
        </div>
      </div>

      {/* Main Navigation Header */}
      <div className="max-w-7xl mx-auto px-4 py-3.5">
        <div className="flex flex-col md:flex-row md:items-center md:justify-between gap-4">
          {/* Brand & Identity */}
          <div className="flex items-center justify-between">
            <div 
              onClick={() => setActiveTab('explorer')}
              className="flex items-center space-x-3 cursor-pointer group"
            >
              <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-amber-600 via-amber-500 to-amber-300 flex items-center justify-center shadow-lg shadow-amber-500/20 ring-1 ring-amber-400/40 group-hover:scale-105 transition-transform">
                <span className="font-serif font-bold text-slate-950 text-xl tracking-tight">स</span>
              </div>
              <div>
                <div className="flex items-center space-x-2">
                  <h1 className="text-xl font-bold font-serif tracking-wide text-white group-hover:text-amber-200 transition-colors">
                    Satya Explorer
                  </h1>
                  <span className="px-1.5 py-0.5 text-[10px] font-semibold uppercase tracking-wider rounded bg-amber-500/10 text-amber-300 border border-amber-500/20">
                    Mainnet
                  </span>
                </div>
                <p className="text-xs text-slate-400">KasturiChain Block & Knowledge Verification</p>
              </div>
            </div>

            {/* Mobile Tab Navigation */}
            <div className="flex md:hidden items-center space-x-1 bg-slate-900/90 p-1 rounded-lg border border-slate-800">
              <button
                onClick={() => setActiveTab('explorer')}
                className={`p-1.5 rounded text-xs font-medium transition-all ${
                  activeTab === 'explorer' 
                    ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30' 
                    : 'text-slate-400 hover:text-white'
                }`}
                title="Explorer"
              >
                <Layers className="w-4 h-4" />
              </button>
              <button
                onClick={() => setActiveTab('manuscripts')}
                className={`p-1.5 rounded text-xs font-medium transition-all ${
                  activeTab === 'manuscripts' 
                    ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30' 
                    : 'text-slate-400 hover:text-white'
                }`}
                title="Vedic Proofs"
              >
                <ScrollText className="w-4 h-4" />
              </button>
              <button
                onClick={() => setActiveTab('devops')}
                className={`p-1.5 rounded text-xs font-medium transition-all ${
                  activeTab === 'devops' 
                    ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30' 
                    : 'text-slate-400 hover:text-white'
                }`}
                title="DevOps & Deploy"
              >
                <Server className="w-4 h-4" />
              </button>
            </div>
          </div>

          {/* Desktop Tab Selector */}
          <div className="hidden md:flex items-center space-x-2 bg-slate-900/90 p-1 rounded-xl border border-slate-800">
            <button
              onClick={() => setActiveTab('explorer')}
              className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-lg text-sm font-medium transition-all ${
                activeTab === 'explorer'
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
              }`}
            >
              <Layers className="w-4 h-4" />
              <span>Explorer</span>
            </button>
            <button
              onClick={() => setActiveTab('manuscripts')}
              className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-lg text-sm font-medium transition-all ${
                activeTab === 'manuscripts'
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
              }`}
            >
              <ScrollText className="w-4 h-4" />
              <span>Vedic Proofs</span>
              <span className="text-[10px] px-1.5 py-0.2 rounded-full bg-emerald-500/20 text-emerald-300 border border-emerald-500/30">
                Live
              </span>
            </button>
            <button
              onClick={() => setActiveTab('devops')}
              className={`flex items-center space-x-2 px-3.5 py-1.5 rounded-lg text-sm font-medium transition-all ${
                activeTab === 'devops'
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30 shadow-sm'
                  : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/60'
              }`}
            >
              <Server className="w-4 h-4" />
              <span>EC2 Deployment</span>
            </button>
          </div>
        </div>

        {/* Omni Search Bar */}
        <div className="mt-3.5">
          <form onSubmit={handleSearchSubmit} className="relative">
            <div className="relative flex items-center">
              <div className="absolute left-3.5 pointer-events-none text-slate-400 flex items-center">
                <Search className="w-4 h-4 text-amber-400/80" />
              </div>

              <input
                type="text"
                value={searchQuery}
                onChange={handleInputChange}
                placeholder="Search by Block Number / Address (0x...) / Tx Hash (0x...) / Manuscript Proof Hash..."
                className="w-full pl-10 pr-32 py-2.5 bg-slate-900/90 hover:bg-slate-900 focus:bg-slate-900 border border-slate-800 focus:border-amber-500/50 rounded-xl text-sm text-slate-100 placeholder-slate-500 focus:outline-none focus:ring-2 focus:ring-amber-500/20 transition-all font-mono"
              />

              {searchTypeHint && (
                <div className="absolute right-24 hidden sm:flex items-center px-2 py-0.5 rounded text-[11px] font-sans font-medium bg-amber-500/10 text-amber-300 border border-amber-500/20">
                  {searchTypeHint}
                </div>
              )}

              <button
                type="submit"
                className="absolute right-1.5 px-4 py-1.5 bg-amber-500 hover:bg-amber-400 text-slate-950 font-semibold rounded-lg text-xs transition-colors shadow-sm flex items-center space-x-1"
              >
                <span>Search</span>
              </button>
            </div>
          </form>
        </div>
      </div>
    </header>
  );
};
