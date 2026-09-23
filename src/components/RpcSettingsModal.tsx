import React, { useState } from 'react';
import { 
  X, 
  Radio, 
  CheckCircle2, 
  AlertCircle, 
  Activity, 
  RotateCw, 
  Check, 
  Server, 
  ShieldCheck 
} from 'lucide-react';
import { DEFAULT_RPC_ENDPOINT, LOCAL_RPC_ENDPOINT, jsonRpcCall } from '../services/rpc';

interface RpcSettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
  currentRpc: string;
  onSelectRpc: (url: string) => void;
}

export const RpcSettingsModal: React.FC<RpcSettingsModalProps> = ({
  isOpen,
  onClose,
  currentRpc,
  onSelectRpc,
}) => {
  const [customInput, setCustomInput] = useState<string>('');
  const [testStatus, setTestStatus] = useState<{
    testing: boolean;
    success?: boolean;
    latency?: number;
    blockNumber?: number;
    error?: string;
  }>({ testing: false });

  if (!isOpen) return null;

  const handleTestEndpoint = async (url: string) => {
    setTestStatus({ testing: true });
    const start = performance.now();
    try {
      const [blockHex, chainHex] = await Promise.all([
        jsonRpcCall<string>('eth_blockNumber', [], url, 4000),
        jsonRpcCall<string>('eth_chainId', [], url, 4000),
      ]);
      const latency = Math.round(performance.now() - start);
      const blockNum = parseInt(blockHex, 16);
      setTestStatus({
        testing: false,
        success: true,
        latency,
        blockNumber: blockNum,
      });
    } catch (err: any) {
      setTestStatus({
        testing: false,
        success: false,
        error: err.message || 'Connection failed',
      });
    }
  };

  const presetOptions = [
    {
      name: 'Yugala Public RPC (Production Upstream)',
      url: DEFAULT_RPC_ENDPOINT,
      description: 'Primary public load-balanced endpoint for KasturiChain',
      isDefault: true,
    },
    {
      name: 'Local Node RPC (EC2 Internal)',
      url: LOCAL_RPC_ENDPOINT,
      description: 'Internal local node at 127.0.0.1:8545 for zero network hops',
      isDefault: false,
    },
  ];

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm">
      <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-lg overflow-hidden shadow-2xl">
        {/* Header */}
        <div className="px-6 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-950/60">
          <div className="flex items-center space-x-2.5">
            <Radio className="w-5 h-5 text-emerald-400" />
            <div>
              <h3 className="text-base font-bold text-white">JSON-RPC Node Configuration</h3>
              <p className="text-xs text-slate-400">Manage client connection to KasturiChain</p>
            </div>
          </div>

          <button
            onClick={onClose}
            className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Content */}
        <div className="p-6 space-y-4">
          <div className="space-y-2.5">
            <label className="text-xs font-semibold uppercase tracking-wider text-slate-400 block">
              Network Presets
            </label>

            {presetOptions.map((opt) => {
              const isSelected = currentRpc === opt.url;
              return (
                <div
                  key={opt.url}
                  onClick={() => {
                    onSelectRpc(opt.url);
                    handleTestEndpoint(opt.url);
                  }}
                  className={`p-3.5 rounded-xl border transition-all cursor-pointer ${
                    isSelected
                      ? 'bg-amber-500/10 border-amber-500/50 shadow-md'
                      : 'bg-slate-950/60 hover:bg-slate-800/40 border-slate-800'
                  }`}
                >
                  <div className="flex items-center justify-between">
                    <div className="font-semibold text-xs text-white flex items-center gap-1.5">
                      <span>{opt.name}</span>
                      {opt.isDefault && (
                        <span className="text-[10px] px-1.5 py-0.2 rounded bg-amber-500/20 text-amber-300">
                          Recommended
                        </span>
                      )}
                    </div>
                    {isSelected && <Check className="w-4 h-4 text-amber-400" />}
                  </div>

                  <div className="text-xs font-mono text-cyan-300 mt-1">{opt.url}</div>
                  <div className="text-[11px] text-slate-400 mt-0.5">{opt.description}</div>
                </div>
              );
            })}
          </div>

          {/* Custom Endpoint Input */}
          <div className="space-y-2 pt-2 border-t border-slate-800">
            <label className="text-xs font-semibold uppercase tracking-wider text-slate-400 block">
              Custom RPC URL
            </label>

            <div className="flex gap-2">
              <input
                type="text"
                value={customInput}
                onChange={(e) => setCustomInput(e.target.value)}
                placeholder="https://..."
                className="flex-1 px-3 py-2 bg-slate-950 border border-slate-800 rounded-xl text-xs font-mono text-slate-200 focus:outline-none focus:border-amber-500/40"
              />
              <button
                onClick={() => {
                  if (customInput.trim()) {
                    onSelectRpc(customInput.trim());
                    handleTestEndpoint(customInput.trim());
                  }
                }}
                className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-white rounded-xl text-xs font-semibold border border-slate-700"
              >
                Apply
              </button>
            </div>
          </div>

          {/* Connection Test Status */}
          <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
            <div className="flex items-center justify-between">
              <span className="text-xs text-slate-400">Endpoint Health Check</span>
              <button
                onClick={() => handleTestEndpoint(currentRpc)}
                disabled={testStatus.testing}
                className="text-xs text-amber-400 hover:underline flex items-center gap-1"
              >
                <RotateCw className={`w-3 h-3 ${testStatus.testing ? 'animate-spin' : ''}`} />
                <span>Test Now</span>
              </button>
            </div>

            {testStatus.testing && (
              <div className="text-xs text-slate-400 mt-2 flex items-center gap-2">
                <Activity className="w-3.5 h-3.5 text-amber-400 animate-pulse" />
                <span>Connecting to {currentRpc}...</span>
              </div>
            )}

            {!testStatus.testing && testStatus.success && (
              <div className="mt-2 text-xs text-emerald-400 flex items-center justify-between">
                <div className="flex items-center gap-1.5">
                  <CheckCircle2 className="w-4 h-4" />
                  <span>Online & Responsive</span>
                </div>
                <div className="font-mono text-[11px] text-slate-300">
                  {testStatus.latency}ms • Block #{testStatus.blockNumber}
                </div>
              </div>
            )}

            {!testStatus.testing && testStatus.error && (
              <div className="mt-2 text-xs text-rose-400 flex items-center gap-1.5">
                <AlertCircle className="w-4 h-4 shrink-0" />
                <span className="truncate">{testStatus.error}</span>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};
