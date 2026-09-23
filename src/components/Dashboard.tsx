import React from 'react';
import { 
  Box, 
  ArrowRight, 
  Clock, 
  User, 
  Send, 
  CheckCircle2, 
  XCircle, 
  Fuel, 
  ExternalLink,
  ChevronRight,
  ShieldCheck
} from 'lucide-react';
import { BlockData, TransactionData } from '../types';
import { shortenHash, timeAgo } from '../services/rpc';

interface DashboardProps {
  blocks: BlockData[];
  transactions: TransactionData[];
  onSelectBlock: (blockNumber: number) => void;
  onSelectTx: (txHash: string) => void;
  onSelectAddress: (address: string) => void;
}

export const Dashboard: React.FC<DashboardProps> = ({
  blocks,
  transactions,
  onSelectBlock,
  onSelectTx,
  onSelectAddress,
}) => {
  return (
    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
      {/* Latest Blocks Column */}
      <div className="bg-slate-900/70 border border-slate-800/80 rounded-2xl overflow-hidden shadow-xl">
        <div className="px-5 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-900/90">
          <div className="flex items-center space-x-2.5">
            <div className="p-2 rounded-lg bg-amber-500/10 text-amber-400 border border-amber-500/20">
              <Box className="w-4 h-4" />
            </div>
            <div>
              <h2 className="text-base font-semibold text-white">Latest Blocks</h2>
              <p className="text-xs text-slate-400">Real-time mined blocks on KasturiChain</p>
            </div>
          </div>
          <span className="text-xs font-mono text-slate-400 bg-slate-800/60 px-2.5 py-1 rounded-full border border-slate-700/60">
            {blocks.length} blocks cached
          </span>
        </div>

        <div className="divide-y divide-slate-800/60 overflow-x-auto">
          {blocks.length === 0 ? (
            <div className="p-8 text-center text-slate-500 text-sm">
              Loading latest blocks from https://rpc.yugala.org...
            </div>
          ) : (
            blocks.slice(0, 8).map((block) => {
              const txCount = block.transactions.length;
              const gasPercent = block.gasLimit > 0 
                ? Math.round((block.gasUsed / block.gasLimit) * 100) 
                : 0;

              return (
                <div
                  key={block.number}
                  onClick={() => onSelectBlock(block.number)}
                  className="px-5 py-3.5 hover:bg-slate-800/40 transition-colors cursor-pointer group flex items-center justify-between gap-4"
                >
                  <div className="flex items-center space-x-3.5 min-w-0">
                    <div className="w-10 h-10 rounded-xl bg-slate-800 group-hover:bg-amber-500/10 border border-slate-700/60 group-hover:border-amber-500/30 flex flex-col items-center justify-center shrink-0 transition-colors">
                      <Box className="w-4 h-4 text-amber-400 group-hover:scale-110 transition-transform" />
                    </div>

                    <div className="min-w-0">
                      <div className="flex items-center space-x-2">
                        <span className="font-mono font-bold text-sm text-amber-400 hover:underline">
                          #{block.number}
                        </span>
                        <span className="text-[11px] text-slate-400 flex items-center gap-1">
                          <Clock className="w-3 h-3" />
                          {timeAgo(block.timestamp)}
                        </span>
                      </div>

                      <div className="text-xs text-slate-400 flex items-center gap-1.5 mt-0.5 truncate">
                        <span>Validator:</span>
                        <span 
                          onClick={(e) => {
                            e.stopPropagation();
                            onSelectAddress(block.miner);
                          }}
                          className="font-mono text-slate-300 hover:text-amber-300 transition-colors"
                        >
                          {shortenHash(block.miner, 6, 4)}
                        </span>
                      </div>
                    </div>
                  </div>

                  <div className="text-right shrink-0 flex items-center space-x-3">
                    <div>
                      <div className="text-xs font-semibold text-slate-200">
                        {txCount} {txCount === 1 ? 'tx' : 'txs'}
                      </div>
                      <div className="text-[11px] text-slate-400 font-mono flex items-center justify-end gap-1">
                        <Fuel className="w-3 h-3 text-slate-400" />
                        <span>{block.gasUsed.toLocaleString()}</span>
                      </div>
                    </div>

                    <ChevronRight className="w-4 h-4 text-slate-600 group-hover:text-amber-400 transition-colors" />
                  </div>
                </div>
              );
            })
          )}
        </div>

        <div className="p-3 bg-slate-900/90 border-t border-slate-800 text-center">
          <span className="text-xs text-slate-400">
            Auto-polling node every 4 seconds • Proof-of-Stake Consensus
          </span>
        </div>
      </div>

      {/* Latest Transactions Column */}
      <div className="bg-slate-900/70 border border-slate-800/80 rounded-2xl overflow-hidden shadow-xl">
        <div className="px-5 py-4 border-b border-slate-800 flex items-center justify-between bg-slate-900/90">
          <div className="flex items-center space-x-2.5">
            <div className="p-2 rounded-lg bg-cyan-500/10 text-cyan-400 border border-cyan-500/20">
              <Send className="w-4 h-4" />
            </div>
            <div>
              <h2 className="text-base font-semibold text-white">Latest Transactions</h2>
              <p className="text-xs text-slate-400">Transfers, contracts & proof anchors</p>
            </div>
          </div>
          <span className="text-xs font-mono text-slate-400 bg-slate-800/60 px-2.5 py-1 rounded-full border border-slate-700/60">
            {transactions.length} transactions
          </span>
        </div>

        <div className="divide-y divide-slate-800/60 overflow-x-auto">
          {transactions.length === 0 ? (
            <div className="p-8 text-center text-slate-500 text-sm">
              Waiting for new transactions on KasturiChain...
            </div>
          ) : (
            transactions.slice(0, 8).map((tx) => {
              const isContractCall = tx.input && tx.input !== '0x';

              return (
                <div
                  key={tx.hash}
                  onClick={() => onSelectTx(tx.hash)}
                  className="px-5 py-3.5 hover:bg-slate-800/40 transition-colors cursor-pointer group flex items-center justify-between gap-4"
                >
                  <div className="flex items-center space-x-3.5 min-w-0">
                    <div className="w-10 h-10 rounded-xl bg-slate-800 group-hover:bg-cyan-500/10 border border-slate-700/60 group-hover:border-cyan-500/30 flex flex-col items-center justify-center shrink-0 transition-colors">
                      <Send className="w-4 h-4 text-cyan-400 group-hover:scale-110 transition-transform" />
                    </div>

                    <div className="min-w-0">
                      <div className="flex items-center space-x-2">
                        <span className="font-mono font-medium text-sm text-cyan-400 hover:underline">
                          {shortenHash(tx.hash, 8, 6)}
                        </span>
                        {tx.timestamp && (
                          <span className="text-[11px] text-slate-400">
                            {timeAgo(tx.timestamp)}
                          </span>
                        )}
                      </div>

                      <div className="text-xs text-slate-400 flex items-center space-x-1.5 mt-0.5 truncate">
                        <span>From</span>
                        <span 
                          onClick={(e) => {
                            e.stopPropagation();
                            onSelectAddress(tx.from);
                          }}
                          className="font-mono text-slate-300 hover:text-amber-300"
                        >
                          {shortenHash(tx.from, 4, 3)}
                        </span>
                        <ArrowRight className="w-3 h-3 text-slate-600" />
                        <span>To</span>
                        <span 
                          onClick={(e) => {
                            e.stopPropagation();
                            if (tx.to) onSelectAddress(tx.to);
                          }}
                          className="font-mono text-slate-300 hover:text-amber-300"
                        >
                          {tx.to ? shortenHash(tx.to, 4, 3) : 'Contract Deploy'}
                        </span>
                      </div>
                    </div>
                  </div>

                  <div className="text-right shrink-0 flex items-center space-x-3">
                    <div>
                      <div className="text-xs font-mono font-semibold text-amber-300">
                        {tx.valueKST} KST
                      </div>
                      <div className="text-[11px] text-emerald-400 flex items-center justify-end gap-1">
                        <CheckCircle2 className="w-3 h-3" />
                        <span>Success</span>
                      </div>
                    </div>

                    <ChevronRight className="w-4 h-4 text-slate-600 group-hover:text-cyan-400 transition-colors" />
                  </div>
                </div>
              );
            })
          )}
        </div>

        <div className="p-3 bg-slate-900/90 border-t border-slate-800 text-center">
          <span className="text-xs text-slate-400">
            Native Currency: KST (18 Decimals) • Shanghai EVM Compatible
          </span>
        </div>
      </div>
    </div>
  );
};
