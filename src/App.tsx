import React, { useState, useEffect, useCallback, useRef } from 'react';
import { 
  getNetworkMetrics, 
  getRecentBlocks, 
  getTransaction, 
  getAccount, 
  DEFAULT_RPC_ENDPOINT, 
  jsonRpcCall,
  parseHexNumber,
  shortenHash 
} from './services/rpc';
import { NetworkMetrics, BlockData, TransactionData, AccountData } from './types';
import { Header } from './components/Header';
import { MetricsBar } from './components/MetricsBar';
import { Dashboard } from './components/Dashboard';
import { ManuscriptPanel } from './components/ManuscriptPanel';
import { BlockModal } from './components/BlockModal';
import { TransactionModal } from './components/TransactionModal';
import { AccountModal } from './components/AccountModal';
import { DevOpsModal } from './components/DevOpsModal';
import { RpcSettingsModal } from './components/RpcSettingsModal';
import { CANONICAL_MANUSCRIPTS } from './data/manuscripts';
import { 
  Server, 
  Layers, 
  ShieldCheck, 
  Radio, 
  AlertCircle, 
  CheckCircle2, 
  ExternalLink,
  Cpu,
  Coins
} from 'lucide-react';

export default function App() {
  const [activeTab, setActiveTab] = useState<'explorer' | 'manuscripts' | 'devops'>('explorer');
  const [rpcEndpoint, setRpcEndpoint] = useState<string>(DEFAULT_RPC_ENDPOINT);
  const [metrics, setMetrics] = useState<NetworkMetrics | null>(null);
  const [blocks, setBlocks] = useState<BlockData[]>([]);
  const [transactions, setTransactions] = useState<TransactionData[]>([]);
  const [isRefreshing, setIsRefreshing] = useState<boolean>(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);

  // Modals state
  const [selectedBlock, setSelectedBlock] = useState<BlockData | null>(null);
  const [selectedTx, setSelectedTx] = useState<TransactionData | null>(null);
  const [selectedAccount, setSelectedAccount] = useState<AccountData | null>(null);
  const [isRpcSettingsOpen, setIsRpcSettingsOpen] = useState<boolean>(false);

  const lastBlockNumberRef = useRef<number>(0);

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 3500);
  };

  /**
   * Main refresh loop: queries node metrics, latest blocks & txs
   */
  const loadChainData = useCallback(async (showSpin: boolean = false) => {
    if (showSpin) setIsRefreshing(true);
    try {
      const netMetrics = await getNetworkMetrics(rpcEndpoint);
      setMetrics(netMetrics);

      // Only re-fetch block list if block height advanced or initial load
      if (netMetrics.blockNumber !== lastBlockNumberRef.current || blocks.length === 0) {
        lastBlockNumberRef.current = netMetrics.blockNumber;
        const recent = await getRecentBlocks(netMetrics.blockNumber, 10, rpcEndpoint);
        setBlocks(recent);

        // Aggregate transactions from recent blocks
        const allTxs: TransactionData[] = [];
        recent.forEach((b) => {
          b.transactions.forEach((tx) => {
            if (typeof tx === 'object') {
              allTxs.push(tx);
            }
          });
        });
        setTransactions(allTxs);
      }
    } catch (err: any) {
      console.warn('Error polling RPC:', err);
    } finally {
      if (showSpin) setIsRefreshing(false);
    }
  }, [rpcEndpoint, blocks.length]);

  // Initial load
  useEffect(() => {
    loadChainData(true);
  }, [loadChainData]);

  // Auto-refresh every 4 seconds as specified in requirements:
  // "Direct Client-Side RPC Polling: Connect directly to https://rpc.yugala.org with zero CORS overhead and auto-refresh every 4 seconds."
  useEffect(() => {
    const interval = setInterval(() => {
      loadChainData(false);
    }, 4000);
    return () => clearInterval(interval);
  }, [loadChainData]);

  // Handlers for selection
  const handleSelectBlock = async (blockNum: number) => {
    const existing = blocks.find((b) => b.number === blockNum);
    if (existing) {
      setSelectedBlock(existing);
      return;
    }
    try {
      showToast(`Fetching Block #${blockNum}...`);
      const hex = '0x' + blockNum.toString(16);
      const raw = await jsonRpcCall('eth_getBlockByNumber', [hex, true], rpcEndpoint);
      if (raw) {
        const blk: BlockData = {
          number: blockNum,
          numberHex: hex,
          hash: raw.hash || '0x' + '0'.repeat(64),
          parentHash: raw.parentHash || '0x' + '0'.repeat(64),
          miner: raw.miner || '0x108108A4E1Bf28325608Ac94B37a67f08B9B1081',
          gasLimit: parseHexNumber(raw.gasLimit) || 30000000,
          gasUsed: parseHexNumber(raw.gasUsed) || 0,
          timestamp: parseHexNumber(raw.timestamp) || Math.floor(Date.now() / 1000),
          transactions: raw.transactions || [],
        };
        setSelectedBlock(blk);
      } else {
        showToast(`Block #${blockNum} not found`);
      }
    } catch (err: any) {
      showToast(`Error fetching block: ${err.message}`);
    }
  };

  const handleSelectTx = async (txHash: string) => {
    const existing = transactions.find((t) => t.hash.toLowerCase() === txHash.toLowerCase());
    if (existing) {
      setSelectedTx(existing);
      return;
    }
    showToast(`Inspecting transaction ${shortenHash(txHash, 6, 4)}...`);
    const tx = await getTransaction(txHash, rpcEndpoint);
    if (tx) {
      setSelectedTx(tx);
    } else {
      showToast(`Transaction ${shortenHash(txHash, 6, 4)} not found`);
    }
  };

  const handleSelectAddress = async (address: string) => {
    showToast(`Loading Account ${shortenHash(address, 6, 4)}...`);
    const acc = await getAccount(address, rpcEndpoint);
    setSelectedAccount(acc);
  };

  /**
   * Omni Search Auto-Detector
   */
  const handleSearch = async (query: string) => {
    const clean = query.trim();
    if (!clean) return;

    // 1. Check if block number (decimal or hex)
    if (/^\d+$/.test(clean)) {
      handleSelectBlock(parseInt(clean, 10));
      return;
    }
    if (clean.startsWith('0x') && clean.length < 10) {
      const num = parseInt(clean, 16);
      if (!isNaN(num)) {
        handleSelectBlock(num);
        return;
      }
    }

    // 2. Check if account/contract address (0x + 40 hex chars)
    if (/^0x[a-fA-F0-9]{40}$/.test(clean)) {
      handleSelectAddress(clean);
      return;
    }

    // 3. Check if tx hash (0x + 64 hex chars)
    if (/^0x[a-fA-F0-9]{64}$/.test(clean)) {
      // Check if it's a manuscript anchor or tx
      const matchedManuscript = CANONICAL_MANUSCRIPTS.find(
        (m) =>
          m.sha256Hash.toLowerCase() === clean.toLowerCase() ||
          m.merkleRoot.toLowerCase() === clean.toLowerCase() ||
          m.anchorTxHash.toLowerCase() === clean.toLowerCase()
      );
      if (matchedManuscript) {
        setActiveTab('manuscripts');
        showToast(`Matched Manuscript: ${matchedManuscript.title}`);
        return;
      }
      handleSelectTx(clean);
      return;
    }

    // 4. Check if manuscript CID or text
    const foundManuscript = CANONICAL_MANUSCRIPTS.find(
      (m) =>
        m.storageCid.toLowerCase().includes(clean.toLowerCase()) ||
        m.title.toLowerCase().includes(clean.toLowerCase()) ||
        m.sanskritTitle.toLowerCase().includes(clean.toLowerCase())
    );
    if (foundManuscript) {
      setActiveTab('manuscripts');
      showToast(`Found Manuscript: ${foundManuscript.title}`);
      return;
    }

    showToast(`No exact match for "${clean}". Try a block number, 0x address, or tx hash.`);
  };

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col selection:bg-amber-500/20 selection:text-amber-200">
      {/* Top Header with Omni Search & Real-Time Status */}
      <Header
        metrics={metrics}
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        onSearch={handleSearch}
        onRefresh={() => loadChainData(true)}
        isRefreshing={isRefreshing}
        rpcEndpoint={rpcEndpoint}
        onOpenRpcSettings={() => setIsRpcSettingsOpen(true)}
      />

      {/* Main App Container */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-4 py-6">
        {activeTab === 'explorer' && (
          <div className="space-y-6">
            {/* Real-time Metrics Bar */}
            <MetricsBar
              metrics={metrics}
              totalBlocks={blocks.length}
              totalTxs={transactions.length}
              onSelectBlock={handleSelectBlock}
              onOpenManuscripts={() => setActiveTab('manuscripts')}
            />

            {/* Main Dual-Column Dashboard (Latest Blocks & Latest Transactions) */}
            <Dashboard
              blocks={blocks}
              transactions={transactions}
              onSelectBlock={handleSelectBlock}
              onSelectTx={handleSelectTx}
              onSelectAddress={handleSelectAddress}
            />
          </div>
        )}

        {activeTab === 'manuscripts' && (
          <ManuscriptPanel
            onSelectTx={handleSelectTx}
            onSelectBlock={handleSelectBlock}
          />
        )}

        {activeTab === 'devops' && (
          <DevOpsModal isStandaloneTab={true} />
        )}
      </main>

      {/* Footer */}
      <footer className="border-t border-slate-800/80 bg-slate-950/80 mt-12 py-8 text-xs text-slate-400">
        <div className="max-w-7xl mx-auto px-4 flex flex-col md:flex-row items-center justify-between gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-6 h-6 rounded-lg bg-amber-500/20 text-amber-300 border border-amber-500/30 flex items-center justify-center font-serif font-bold text-xs">
              स
            </div>
            <div>
              <span className="font-semibold text-slate-200">Satya Explorer</span>
              <span className="mx-2">•</span>
              <span>KasturiChain Network (Chain ID 108108 / 0x1a64c)</span>
            </div>
          </div>

          <div className="flex flex-wrap items-center gap-4 text-[11px] font-mono">
            <span className="text-slate-500">Host: 52.72.236.75 (EC2)</span>
            <span className="text-slate-500">Domain: satya.kasturisundari.xyz</span>
            <span className="text-emerald-400 flex items-center gap-1">
              <span className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse"></span>
              <span>4s Direct Client-RPC Polling</span>
            </span>
          </div>
        </div>
      </footer>

      {/* Modals */}
      <BlockModal
        block={selectedBlock}
        isOpen={!!selectedBlock}
        onClose={() => setSelectedBlock(null)}
        onSelectTx={handleSelectTx}
        onSelectAddress={handleSelectAddress}
        onNavigateBlock={handleSelectBlock}
      />

      <TransactionModal
        tx={selectedTx}
        isOpen={!!selectedTx}
        onClose={() => setSelectedTx(null)}
        onSelectAddress={handleSelectAddress}
        onSelectBlock={handleSelectBlock}
      />

      <AccountModal
        account={selectedAccount}
        isOpen={!!selectedAccount}
        onClose={() => setSelectedAccount(null)}
      />

      <RpcSettingsModal
        isOpen={isRpcSettingsOpen}
        onClose={() => setIsRpcSettingsOpen(false)}
        currentRpc={rpcEndpoint}
        onSelectRpc={(newRpc) => {
          setRpcEndpoint(newRpc);
          setIsRpcSettingsOpen(false);
          showToast(`Switched RPC to ${newRpc}`);
        }}
      />

      {/* Toast notifications */}
      {toastMessage && (
        <div className="fixed bottom-5 right-5 z-50 bg-slate-900 border border-amber-500/40 text-amber-200 px-4 py-2.5 rounded-xl text-xs font-medium shadow-2xl flex items-center space-x-2 animate-in fade-in">
          <CheckCircle2 className="w-4 h-4 text-amber-400 shrink-0" />
          <span>{toastMessage}</span>
        </div>
      )}
    </div>
  );
}
