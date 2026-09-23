import React, { useState } from 'react';
import { 
  X, 
  User, 
  Coins, 
  Copy, 
  FileCode, 
  Layers, 
  CheckCircle2, 
  ShieldCheck, 
  Binary, 
  ExternalLink,
  Code2
} from 'lucide-react';
import { AccountData } from '../types';
import { shortenHash } from '../services/rpc';

interface AccountModalProps {
  account: AccountData | null;
  isOpen: boolean;
  onClose: () => void;
  onSelectTx?: (txHash: string) => void;
}

export const AccountModal: React.FC<AccountModalProps> = ({
  account,
  isOpen,
  onClose,
}) => {
  const [copiedField, setCopiedField] = useState<string | null>(null);
  const [showBytecode, setShowBytecode] = useState<boolean>(false);

  if (!isOpen || !account) return null;

  const copyToClipboard = (text: string, label: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(label);
    setTimeout(() => setCopiedField(null), 2000);
  };

  const isContract = account.isContract;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/80 backdrop-blur-sm overflow-y-auto">
      <div className="bg-slate-900 border border-slate-800 rounded-2xl w-full max-w-3xl overflow-hidden shadow-2xl my-8">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-950/60">
          <div className="flex items-center space-x-3">
            <div className={`p-2.5 rounded-xl border ${isContract ? 'bg-amber-500/10 text-amber-400 border-amber-500/20' : 'bg-cyan-500/10 text-cyan-400 border-cyan-500/20'}`}>
              {isContract ? <Code2 className="w-5 h-5" /> : <User className="w-5 h-5" />}
            </div>
            <div>
              <div className="flex items-center space-x-2">
                <h3 className="text-lg font-bold text-white">
                  {isContract ? 'Smart Contract Details' : 'Account Details'}
                </h3>
                <span className={`px-2 py-0.5 text-[10px] font-semibold uppercase rounded-full border ${
                  isContract ? 'bg-amber-500/10 text-amber-300 border-amber-500/30' : 'bg-emerald-500/10 text-emerald-300 border-emerald-500/30'
                }`}>
                  {isContract ? 'Contract' : 'EOA Wallet'}
                </span>
              </div>
              <p className="text-xs text-slate-400">KasturiChain Network Entity</p>
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
        <div className="p-6 space-y-5 max-h-[75vh] overflow-y-auto">
          {/* Address Box */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 flex items-center justify-between gap-3 text-xs font-mono">
            <div className="min-w-0">
              <span className="text-slate-400 block text-[10px] font-sans">
                {isContract ? 'Contract Address' : 'Account Address (0x...)'}
              </span>
              <span className="text-amber-300 font-bold break-all block mt-0.5 text-xs sm:text-sm">
                {account.address}
              </span>
            </div>
            <button
              onClick={() => copyToClipboard(account.address, 'Address')}
              className="p-2 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
              title="Copy Address"
            >
              <Copy className="w-4 h-4" />
            </button>
          </div>

          {/* Balance & Nonce */}
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
            <div className="bg-slate-950 p-4 rounded-xl border border-slate-800">
              <span className="text-[11px] uppercase tracking-wider text-slate-400 block mb-1">
                KST Balance (eth_getBalance)
              </span>
              <div className="flex items-baseline space-x-2">
                <span className="text-2xl font-bold font-mono text-white">
                  {account.balanceKST}
                </span>
                <span className="text-sm font-semibold text-amber-400">KST</span>
              </div>
              <div className="text-[11px] text-slate-500 font-mono mt-1 truncate">
                Wei: {account.balanceWei}
              </div>
            </div>

            <div className="bg-slate-950 p-4 rounded-xl border border-slate-800">
              <span className="text-[11px] uppercase tracking-wider text-slate-400 block mb-1">
                Nonce / Transaction Count
              </span>
              <div className="flex items-baseline space-x-2">
                <span className="text-2xl font-bold font-mono text-cyan-300">
                  {account.transactionCount}
                </span>
                <span className="text-xs text-slate-400">total transactions sent</span>
              </div>
              <div className="text-[11px] text-slate-500 font-mono mt-1">
                eth_getTransactionCount [latest]
              </div>
            </div>
          </div>

          {/* Smart Contract Bytecode View (if contract) */}
          {isContract && (
            <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-3">
              <div className="flex items-center justify-between">
                <div className="flex items-center space-x-2">
                  <FileCode className="w-4 h-4 text-amber-400" />
                  <span className="text-xs font-semibold uppercase tracking-wider text-slate-300">
                    Deployed Bytecode (eth_getCode)
                  </span>
                </div>

                <div className="flex items-center space-x-2">
                  <span className="text-[11px] font-mono text-slate-400">
                    {account.bytecode ? Math.floor((account.bytecode.length - 2) / 2) : 0} bytes
                  </span>
                  <button
                    onClick={() => copyToClipboard(account.bytecode || '0x', 'Bytecode')}
                    className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white"
                    title="Copy Bytecode"
                  >
                    <Copy className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>

              <div className="bg-slate-900/90 p-3 rounded-lg border border-slate-800 font-mono text-xs text-slate-300 break-all max-h-48 overflow-y-auto leading-relaxed">
                {account.bytecode}
              </div>
            </div>
          )}

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
