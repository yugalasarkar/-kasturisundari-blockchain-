import React from 'react';
import { 
  X, 
  Box, 
  Clock, 
  Fuel, 
  Copy, 
  Send, 
  ArrowRight, 
  ShieldCheck, 
  CheckCircle2, 
  ChevronRight,
  ExternalLink
} from 'lucide-react';
import { BlockData, TransactionData } from '../types';
import { shortenHash, timeAgo } from '../services/rpc';

interface BlockModalProps {
  block: BlockData | null;
  isOpen: boolean;
  onClose: () => void;
  onSelectTx: (txHash: string) => void;
  onSelectAddress: (address: string) => void;
  onNavigateBlock: (blockNumber: number) => void;
}

export const BlockModal: React.FC<BlockModalProps> = ({
  block,
  isOpen,
  onClose,
  onSelectTx,
  onSelectAddress,
  onNavigateBlock,
}) => {
  const [copiedField, setCopiedField] = React.useState<string | null>(null);

  if (!isOpen || !block) return null;

  const copyToClipboard = (text: string, label: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(label);
    setTimeout(() => setCopiedField(null), 2000);
  };

  const gasPercentage = block.gasLimit > 0 ? ((block.gasUsed / block.gasLimit) * 100).toFixed(2) : '0';

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-y-auto">
      <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-3xl overflow-hidden shadow-2xl my-8">
        {/* Header */}
        <div className="px-6 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-950/60">
          <div className="flex items-center space-x-3">
            <div className="p-2 rounded-xl bg-amber-500/10 text-amber-400 border border-amber-500/20">
              <Box className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h3 className="text-lg font-bold text-white">Block #{block.number}</h3>
                <span className="text-xs font-mono text-slate-400">({block.numberHex})</span>
              </div>
              <p className="text-xs text-slate-400">KasturiChain Finalized Block Details</p>
            </div>
          </div>

          <div className="flex items-center space-x-2">
            <div className="flex items-center space-x-1 mr-2 text-xs">
              <button
                onClick={() => onNavigateBlock(block.number - 1)}
                disabled={block.number <= 0}
                className="px-2 py-1 bg-slate-800 hover:bg-slate-700 disabled:opacity-40 text-slate-300 rounded border border-slate-700/60 transition-colors"
              >
                Prev #{block.number - 1}
              </button>
              <button
                onClick={() => onNavigateBlock(block.number + 1)}
                className="px-2 py-1 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded border border-slate-700/60 transition-colors"
              >
                Next #{block.number + 1}
              </button>
            </div>

            <button
              onClick={onClose}
              className="p-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-400 hover:text-white transition-colors"
            >
              <X className="w-5 h-5" />
            </button>
          </div>
        </div>

        {/* Content Body */}
        <div className="p-6 space-y-4 max-h-[75vh] overflow-y-auto">
          {/* Key Attributes */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
              <span className="text-[11px] uppercase tracking-wider text-slate-400 block mb-1">
                Timestamp
              </span>
              <div className="flex items-center space-x-2 text-sm text-slate-200">
                <Clock className="w-4 h-4 text-amber-400" />
                <span>{new Date(block.timestamp * 1000).toUTCString()}</span>
              </div>
              <span className="text-xs text-slate-400 block mt-1">({timeAgo(block.timestamp)})</span>
            </div>

            <div className="bg-slate-950 p-3.5 rounded-xl border border-slate-800/80">
              <span className="text-[11px] uppercase tracking-wider text-slate-400 block mb-1">
                Transactions
              </span>
              <div className="text-sm font-semibold text-white">
                {block.transactions.length} transactions in this block
              </div>
              <span className="text-xs text-slate-400 block mt-1">
                Gas Used: {block.gasUsed.toLocaleString()} ({gasPercentage}%)
              </span>
            </div>
          </div>

          {/* Block Hashes */}
          <div className="space-y-2.5 text-xs font-mono">
            {/* Hash */}
            <div className="bg-slate-950 p-3 rounded-xl border border-slate-800/80 flex items-center justify-between gap-3">
              <div className="min-w-0">
                <span className="text-slate-400 block text-[10px] font-sans">Block Hash</span>
                <span className="text-amber-300 truncate block text-xs">{block.hash}</span>
              </div>
              <button
                onClick={() => copyToClipboard(block.hash, 'Block Hash')}
                className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
              >
                <Copy className="w-3.5 h-3.5" />
              </button>
            </div>

            {/* Parent Hash */}
            <div className="bg-slate-950 p-3 rounded-xl border border-slate-800/80 flex items-center justify-between gap-3">
              <div className="min-w-0">
                <span className="text-slate-400 block text-[10px] font-sans">Parent Hash</span>
                <span className="text-slate-300 truncate block text-xs">{block.parentHash}</span>
              </div>
              <button
                onClick={() => copyToClipboard(block.parentHash, 'Parent Hash')}
                className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
              >
                <Copy className="w-3.5 h-3.5" />
              </button>
            </div>

            {/* Miner / Validator */}
            <div className="bg-slate-950 p-3 rounded-xl border border-slate-800/80 flex items-center justify-between gap-3">
              <div className="min-w-0">
                <span className="text-slate-400 block text-[10px] font-sans">Mined / Validated By</span>
                <button
                  onClick={() => onSelectAddress(block.miner)}
                  className="text-cyan-400 hover:underline truncate block text-xs"
                >
                  {block.miner}
                </button>
              </div>
              <button
                onClick={() => copyToClipboard(block.miner, 'Validator Address')}
                className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
              >
                <Copy className="w-3.5 h-3.5" />
              </button>
            </div>
          </div>

          {/* Gas & Technical Specs */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800/80 space-y-3">
            <div className="flex items-center justify-between text-xs">
              <span className="text-slate-400">Gas Usage</span>
              <span className="font-mono text-slate-200 font-semibold">
                {block.gasUsed.toLocaleString()} / {block.gasLimit.toLocaleString()} ({gasPercentage}%)
              </span>
            </div>

            {/* Gas Progress Bar */}
            <div className="w-full h-2 bg-slate-800 rounded-full overflow-hidden">
              <div 
                className="h-full bg-gradient-to-r from-emerald-500 to-amber-500 rounded-full"
                style={{ width: `${Math.min(100, parseFloat(gasPercentage) || 5)}%` }}
              ></div>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-3 gap-2 text-xs pt-2 font-mono text-slate-400">
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Block Size</span>
                <span className="text-slate-200">{block.size || 1024} bytes</span>
              </div>
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Extra Data</span>
                <span className="text-slate-200 truncate block">{block.extraData || 'KasturiChain'}</span>
              </div>
              <div>
                <span className="block text-[10px] text-slate-500 font-sans">Nonce</span>
                <span className="text-slate-200">{block.nonce || '0x0'}</span>
              </div>
            </div>
          </div>

          {/* Transactions List */}
          <div className="space-y-2">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-400 flex items-center gap-1.5">
              <Send className="w-3.5 h-3.5 text-cyan-400" />
              <span>Transactions in Block ({block.transactions.length})</span>
            </h4>

            {block.transactions.length === 0 ? (
              <div className="p-4 bg-slate-950 rounded-xl border border-slate-800/80 text-center text-xs text-slate-500">
                No user transactions recorded in this block (Consensus heartbeat).
              </div>
            ) : (
              <div className="space-y-1.5 max-h-48 overflow-y-auto">
                {block.transactions.map((item, idx) => {
                  const txHash = typeof item === 'string' ? item : item.hash;
                  const txValue = typeof item === 'object' ? item.valueKST : '0';
                  const txFrom = typeof item === 'object' ? item.from : null;
                  const txTo = typeof item === 'object' ? item.to : null;

                  return (
                    <div
                      key={txHash || idx}
                      onClick={() => onSelectTx(txHash)}
                      className="p-2.5 bg-slate-950 hover:bg-slate-800/60 rounded-xl border border-slate-800 flex items-center justify-between gap-2 cursor-pointer transition-colors"
                    >
                      <div className="flex items-center space-x-2 min-w-0">
                        <Send className="w-3 h-3 text-cyan-400 shrink-0" />
                        <span className="font-mono text-xs text-cyan-300 hover:underline truncate">
                          {shortenHash(txHash, 10, 8)}
                        </span>
                      </div>

                      <div className="flex items-center space-x-3 text-xs shrink-0">
                        {txValue && (
                          <span className="font-mono text-amber-300 font-medium">
                            {txValue} KST
                          </span>
                        )}
                        <ChevronRight className="w-3.5 h-3.5 text-slate-500" />
                      </div>
                    </div>
                  );
                })}
              </div>
            )}
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
