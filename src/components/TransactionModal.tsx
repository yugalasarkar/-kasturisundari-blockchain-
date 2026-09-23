import React, { useState } from 'react';
import { 
  X, 
  Send, 
  CheckCircle2, 
  XCircle, 
  Clock, 
  Fuel, 
  Copy, 
  ArrowRight, 
  Layers, 
  FileCode, 
  Binary, 
  ShieldCheck,
  ExternalLink 
} from 'lucide-react';
import { TransactionData } from '../types';
import { shortenHash, hexToUtf8, timeAgo } from '../services/rpc';

interface TransactionModalProps {
  tx: TransactionData | null;
  isOpen: boolean;
  onClose: () => void;
  onSelectAddress: (address: string) => void;
  onSelectBlock: (blockNumber: number) => void;
}

export const TransactionModal: React.FC<TransactionModalProps> = ({
  tx,
  isOpen,
  onClose,
  onSelectAddress,
  onSelectBlock,
}) => {
  const [copiedField, setCopiedField] = useState<string | null>(null);
  const [inputViewMode, setInputViewMode] = useState<'hex' | 'utf8'>('hex');

  if (!isOpen || !tx) return null;

  const copyToClipboard = (text: string, label: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(label);
    setTimeout(() => setCopiedField(null), 2000);
  };

  const isSuccess = tx.status === 'success' || (tx.receipt ? tx.receipt.status : true);
  const gasFeeKST = tx.gasPriceWei 
    ? ((BigInt(tx.gasPriceWei) * BigInt(tx.gasUsed || tx.gas || 21000)) / BigInt(1e9)).toString()
    : '21000';
  const decodedInput = tx.input && tx.input !== '0x' ? hexToUtf8(tx.input) : '';

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-y-auto">
      <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-3xl overflow-hidden shadow-2xl my-8">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-950/60">
          <div className="flex items-center space-x-3">
            <div className={`p-2 rounded-xl border ${isSuccess ? 'bg-emerald-500/10 text-emerald-400 border-emerald-500/20' : 'bg-rose-500/10 text-rose-400 border-rose-500/20'}`}>
              <Send className="w-5 h-5" />
            </div>
            <div>
              <h3 className="text-lg font-bold text-white">Transaction Details</h3>
              <p className="text-xs text-slate-400">KasturiChain EVM State Execution</p>
            </div>
          </div>

          <button
            onClick={onClose}
            className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        {/* Modal Body */}
        <div className="p-6 space-y-4 max-h-[75vh] overflow-y-auto">
          {/* Status & Value Row */}
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-3">
            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
              <span className="text-[10px] uppercase tracking-wider text-slate-400 block mb-1">
                Execution Status
              </span>
              <div className="flex items-center space-x-2">
                {isSuccess ? (
                  <>
                    <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                    <span className="text-sm font-semibold text-emerald-300">Success</span>
                  </>
                ) : (
                  <>
                    <XCircle className="w-4 h-4 text-rose-400" />
                    <span className="text-sm font-semibold text-rose-300">Failed</span>
                  </>
                )}
              </div>
            </div>

            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
              <span className="text-[10px] uppercase tracking-wider text-slate-400 block mb-1">
                Block Number
              </span>
              <button
                onClick={() => onSelectBlock(tx.blockNumber)}
                className="text-sm font-mono font-bold text-amber-400 hover:underline flex items-center gap-1"
              >
                #{tx.blockNumber} <ExternalLink className="w-3 h-3" />
              </button>
            </div>

            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
              <span className="text-[10px] uppercase tracking-wider text-slate-400 block mb-1">
                Amount Transferred
              </span>
              <div className="text-sm font-mono font-bold text-amber-300">
                {tx.valueKST} KST
              </div>
            </div>
          </div>

          {/* Transaction Hash */}
          <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80 flex items-center justify-between gap-3 text-xs font-mono">
            <div className="min-w-0">
              <span className="text-slate-400 block text-[10px] font-sans">Transaction Hash</span>
              <span className="text-cyan-300 truncate block text-xs">{tx.hash}</span>
            </div>
            <button
              onClick={() => copyToClipboard(tx.hash, 'Tx Hash')}
              className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
              title="Copy Tx Hash"
            >
              <Copy className="w-3.5 h-3.5" />
            </button>
          </div>

          {/* From -> To */}
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 text-xs font-mono">
            {/* From */}
            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80 space-y-1">
              <span className="text-slate-400 block text-[10px] font-sans">Sender (From)</span>
              <div className="flex items-center justify-between gap-2">
                <button
                  onClick={() => onSelectAddress(tx.from)}
                  className="text-slate-200 hover:text-amber-300 truncate block text-xs"
                >
                  {tx.from}
                </button>
                <button
                  onClick={() => copyToClipboard(tx.from, 'From Address')}
                  className="p-1 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
                >
                  <Copy className="w-3 h-3" />
                </button>
              </div>
            </div>

            {/* To */}
            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80 space-y-1">
              <span className="text-slate-400 block text-[10px] font-sans">
                Recipient / Contract (To)
              </span>
              <div className="flex items-center justify-between gap-2">
                {tx.to ? (
                  <>
                    <button
                      onClick={() => onSelectAddress(tx.to!)}
                      className="text-slate-200 hover:text-amber-300 truncate block text-xs"
                    >
                      {tx.to}
                    </button>
                    <button
                      onClick={() => copyToClipboard(tx.to!, 'To Address')}
                      className="p-1 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
                    >
                      <Copy className="w-3 h-3" />
                    </button>
                  </>
                ) : (
                  <span className="text-amber-400 font-sans font-semibold text-xs">
                    Contract Creation [Deploy]
                  </span>
                )}
              </div>
            </div>
          </div>

          {/* Gas & Fees Breakdown */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800/80 space-y-3 text-xs">
            <h4 className="font-semibold uppercase tracking-wider text-slate-400 flex items-center gap-1.5 text-[11px]">
              <Fuel className="w-3.5 h-3.5 text-amber-400" />
              <span>Gas & Nonce Metrics</span>
            </h4>

            <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 font-mono">
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Gas Used</span>
                <span className="text-slate-200">{(tx.gasUsed || 21000).toLocaleString()}</span>
              </div>
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Gas Limit</span>
                <span className="text-slate-200">{tx.gas.toLocaleString()}</span>
              </div>
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Gas Price</span>
                <span className="text-slate-200">{tx.gasPriceGwei || 1} Gwei</span>
              </div>
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Nonce</span>
                <span className="text-slate-200">{tx.nonce}</span>
              </div>
            </div>
          </div>

          {/* Input Data / Payload View */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800/80 space-y-2">
            <div className="flex items-center justify-between">
              <span className="text-[11px] font-semibold uppercase tracking-wider text-slate-400 flex items-center gap-1.5">
                <FileCode className="w-3.5 h-3.5 text-cyan-400" />
                <span>Input Data Payload</span>
              </span>

              <div className="flex items-center space-x-1 bg-slate-900 p-0.5 rounded border border-slate-800 text-[11px]">
                <button
                  onClick={() => setInputViewMode('hex')}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    inputViewMode === 'hex' ? 'bg-cyan-500/20 text-cyan-300 font-semibold' : 'text-slate-400'
                  }`}
                >
                  Hex
                </button>
                <button
                  onClick={() => setInputViewMode('utf8')}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    inputViewMode === 'utf8' ? 'bg-cyan-500/20 text-cyan-300 font-semibold' : 'text-slate-400'
                  }`}
                >
                  UTF-8
                </button>
              </div>
            </div>

            <div className="bg-slate-900/90 p-3 rounded-lg border border-slate-800/80 font-mono text-xs text-slate-300 break-all max-h-32 overflow-y-auto">
              {inputViewMode === 'hex' ? (
                tx.input || '0x'
              ) : decodedInput ? (
                decodedInput
              ) : (
                <span className="text-slate-500 italic">No human-readable UTF-8 data</span>
              )}
            </div>
          </div>

          {copiedField && (
            <div className="text-center text-xs text-emerald-400 bg-emerald-950/40 border border-emerald-500/30 py-1.5 rounded-lg">
              Copied {copiedField} to clipboard!
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
