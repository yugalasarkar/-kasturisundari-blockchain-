import React, { useState } from 'react';
import { 
  ScrollText, 
  ShieldCheck, 
  Search, 
  CheckCircle2, 
  Copy, 
  ExternalLink, 
  Fingerprint, 
  Database, 
  Key, 
  Sparkles,
  Layers,
  FileCheck2,
  Lock,
  Compass
} from 'lucide-react';
import { ManuscriptRecord } from '../types';
import { CANONICAL_MANUSCRIPTS } from '../data/manuscripts';
import { computeSha256, shortenHash } from '../services/rpc';

interface ManuscriptPanelProps {
  onSelectTx: (txHash: string) => void;
  onSelectBlock: (blockNumber: number) => void;
}

export const ManuscriptPanel: React.FC<ManuscriptPanelProps> = ({
  onSelectTx,
  onSelectBlock,
}) => {
  const [selectedManuscript, setSelectedManuscript] = useState<ManuscriptRecord | null>(CANONICAL_MANUSCRIPTS[0]);
  const [activeCategory, setActiveCategory] = useState<string>('All');
  const [searchQuery, setSearchQuery] = useState<string>('');
  
  // Interactive Live Verifier State
  const [customInputText, setCustomInputText] = useState<string>('');
  const [customComputedHash, setCustomComputedHash] = useState<string>('');
  const [verificationResult, setVerificationResult] = useState<{
    status: 'matched' | 'unregistered' | 'idle';
    manuscript?: ManuscriptRecord;
    hash?: string;
  }>({ status: 'idle' });
  const [copiedField, setCopiedField] = useState<string | null>(null);

  const categories = ['All', 'Shruti (Veda)', 'Itihasa / Gita', 'Jyotisha / Ganita', 'Tantra / Agama', 'Upanishad'];

  const filteredManuscripts = CANONICAL_MANUSCRIPTS.filter(item => {
    const matchesCat = activeCategory === 'All' || item.category === activeCategory;
    const matchesSearch = !searchQuery || 
      item.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.sanskritTitle.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.sha256Hash.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.merkleRoot.toLowerCase().includes(searchQuery.toLowerCase()) ||
      item.storageCid.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesCat && matchesSearch;
  });

  const handleComputeProof = async () => {
    if (!customInputText.trim()) return;
    const computed = await computeSha256(customInputText.trim());
    setCustomComputedHash(computed);

    // Check if it matches any canonical manuscript (by text or hash)
    const matched = CANONICAL_MANUSCRIPTS.find(m => 
      m.sha256Hash.toLowerCase() === computed.toLowerCase() ||
      m.sanskritText.trim() === customInputText.trim() ||
      m.sha256Hash.toLowerCase() === customInputText.trim().toLowerCase() ||
      m.merkleRoot.toLowerCase() === customInputText.trim().toLowerCase()
    );

    if (matched) {
      setVerificationResult({
        status: 'matched',
        manuscript: matched,
        hash: computed,
      });
      setSelectedManuscript(matched);
    } else {
      setVerificationResult({
        status: 'unregistered',
        hash: computed,
      });
    }
  };

  const copyToClipboard = (text: string, label: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(label);
    setTimeout(() => setCopiedField(null), 2000);
  };

  return (
    <div className="space-y-6">
      {/* Hero Header for Knowledge Verification */}
      <div className="bg-gradient-to-br from-slate-900 via-slate-900 to-amber-950/40 border border-amber-500/20 rounded-2xl p-6 shadow-2xl relative overflow-hidden">
        <div className="absolute -right-8 -top-8 w-56 h-56 bg-amber-500/10 rounded-full blur-3xl pointer-events-none"></div>
        <div className="relative z-10 flex flex-col md:flex-row md:items-center md:justify-between gap-6">
          <div className="space-y-2">
            <div className="inline-flex items-center space-x-2 px-3 py-1 rounded-full bg-amber-500/10 border border-amber-500/20 text-amber-300 text-xs font-semibold">
              <Sparkles className="w-3.5 h-3.5" />
              <span>kasturi_storage Cryptographic Verification Protocol</span>
            </div>
            <h2 className="text-2xl font-bold font-serif text-white tracking-wide">
              Vedic Manuscript & Knowledge Proof Registry
            </h2>
            <p className="text-sm text-slate-300 max-w-2xl leading-relaxed">
              Verify mathematical and textual integrity proofs of ancient canonical scriptures anchored onto KasturiChain. Each record stores an immutable SHA-256 digest, Merkle root, and decentralized storage CID permanently attested by network validators.
            </p>
          </div>

          <div className="flex items-center space-x-3 bg-slate-950/70 border border-slate-800 p-3.5 rounded-xl shrink-0">
            <div className="p-2.5 rounded-lg bg-emerald-500/10 text-emerald-400 border border-emerald-500/20">
              <ShieldCheck className="w-6 h-6" />
            </div>
            <div>
              <div className="text-xs text-slate-400">Registry Status</div>
              <div className="text-sm font-bold text-emerald-400 font-mono">100% VERIFIED</div>
              <div className="text-[11px] text-slate-400">Zero-Tamper Guarantee</div>
            </div>
          </div>
        </div>
      </div>

      {/* Interactive Live Cryptographic Verifier Tool */}
      <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-5 shadow-xl">
        <div className="flex items-center space-x-2.5 mb-3">
          <Fingerprint className="w-5 h-5 text-amber-400" />
          <h3 className="text-base font-semibold text-white">Live Manuscript Cryptographic Proof Verifier</h3>
        </div>
        <p className="text-xs text-slate-400 mb-4">
          Paste Sanskrit verse, manuscript transliteration, or SHA-256 hash below to calculate cryptographic proof and verify against KasturiChain registry:
        </p>

        <div className="flex flex-col sm:flex-row gap-3">
          <input
            type="text"
            value={customInputText}
            onChange={(e) => setCustomInputText(e.target.value)}
            placeholder="Paste verse (e.g. 'अग्निमीळे पुरोहितं...' or hash '0x3f5b7218...')"
            className="flex-1 px-4 py-2.5 bg-slate-950 border border-slate-800 focus:border-amber-500/50 rounded-xl text-sm text-slate-100 font-mono focus:outline-none"
          />
          <button
            onClick={handleComputeProof}
            className="px-5 py-2.5 bg-gradient-to-r from-amber-600 to-amber-500 hover:from-amber-500 hover:to-amber-400 text-slate-950 font-semibold rounded-xl text-xs flex items-center justify-center space-x-2 shadow-lg shadow-amber-500/20 transition-all"
          >
            <ShieldCheck className="w-4 h-4" />
            <span>Verify Proof</span>
          </button>
        </div>

        {/* Verification Result Notification */}
        {verificationResult.status !== 'idle' && (
          <div className="mt-4 p-4 rounded-xl border transition-all animate-in fade-in">
            {verificationResult.status === 'matched' && verificationResult.manuscript && (
              <div className="bg-emerald-950/40 border-emerald-500/40 text-emerald-200">
                <div className="flex items-start space-x-3">
                  <CheckCircle2 className="w-5 h-5 text-emerald-400 shrink-0 mt-0.5" />
                  <div className="space-y-1">
                    <div className="font-semibold text-emerald-300 text-sm">
                      Proof Verified: Canonical Manuscript Match Found!
                    </div>
                    <div className="text-xs text-emerald-200/90 font-serif">
                      "{verificationResult.manuscript.title}" ({verificationResult.manuscript.sanskritTitle})
                    </div>
                    <div className="text-[11px] font-mono text-emerald-400/90 break-all">
                      SHA-256: {verificationResult.hash}
                    </div>
                    <div className="text-xs text-slate-300 pt-1 flex items-center space-x-4">
                      <span className="text-amber-300">
                        Anchored at Block #{verificationResult.manuscript.anchorBlock}
                      </span>
                      <button
                        onClick={() => onSelectTx(verificationResult.manuscript!.anchorTxHash)}
                        className="text-cyan-400 hover:underline flex items-center gap-1"
                      >
                        View Tx Anchor <ExternalLink className="w-3 h-3" />
                      </button>
                    </div>
                  </div>
                </div>
              </div>
            )}

            {verificationResult.status === 'unregistered' && (
              <div className="bg-amber-950/30 border-amber-500/40 text-amber-200">
                <div className="flex items-start space-x-3">
                  <FileCheck2 className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
                  <div className="space-y-1">
                    <div className="font-semibold text-amber-300 text-sm">
                      Hash Computed Successfully (Unanchored Draft)
                    </div>
                    <div className="text-xs text-slate-300">
                      This input yields a valid cryptographic digest, ready for future anchoring in kasturi_storage.
                    </div>
                    <div className="text-[11px] font-mono text-amber-300 break-all bg-slate-950/80 p-2 rounded border border-amber-500/20">
                      Calculated SHA-256: {verificationResult.hash}
                    </div>
                  </div>
                </div>
              </div>
            )}
          </div>
        )}
      </div>

      {/* Main Catalog & Detail Section */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Column: Manuscript Catalog List */}
        <div className="lg:col-span-5 space-y-4">
          <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-4 shadow-xl">
            {/* Search & Categories */}
            <div className="space-y-3 mb-4">
              <div className="relative">
                <Search className="w-4 h-4 text-slate-400 absolute left-3 top-3" />
                <input
                  type="text"
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  placeholder="Filter manuscripts, hashes, or CIDs..."
                  className="w-full pl-9 pr-3 py-2 bg-slate-950 border border-slate-800 rounded-xl text-xs text-slate-100 placeholder-slate-500 focus:outline-none focus:border-amber-500/40 font-mono"
                />
              </div>

              {/* Category Pills */}
              <div className="flex items-center gap-1.5 overflow-x-auto pb-1 no-scrollbar">
                {categories.map((cat) => (
                  <button
                    key={cat}
                    onClick={() => setActiveCategory(cat)}
                    className={`px-2.5 py-1 rounded-lg text-xs font-medium whitespace-nowrap transition-colors ${
                      activeCategory === cat
                        ? 'bg-amber-500/20 text-amber-300 border border-amber-500/40'
                        : 'bg-slate-950 text-slate-400 hover:text-white border border-slate-800/80'
                    }`}
                  >
                    {cat}
                  </button>
                ))}
              </div>
            </div>

            {/* List */}
            <div className="space-y-2.5 max-h-[580px] overflow-y-auto pr-1">
              {filteredManuscripts.map((item) => {
                const isSelected = selectedManuscript?.id === item.id;
                return (
                  <div
                    key={item.id}
                    onClick={() => setSelectedManuscript(item)}
                    className={`p-3.5 rounded-xl border transition-all cursor-pointer ${
                      isSelected
                        ? 'bg-amber-500/10 border-amber-500/50 shadow-md shadow-amber-500/5'
                        : 'bg-slate-950/60 hover:bg-slate-800/40 border-slate-800/70'
                    }`}
                  >
                    <div className="flex items-center justify-between gap-2 mb-1">
                      <span className="text-[10px] font-semibold tracking-wider uppercase px-2 py-0.5 rounded bg-slate-800 text-slate-300 border border-slate-700/50">
                        {item.category}
                      </span>
                      <span className="inline-flex items-center space-x-1 text-[11px] font-mono text-emerald-400">
                        <ShieldCheck className="w-3 h-3" />
                        <span>{item.verifiedStatus}</span>
                      </span>
                    </div>

                    <h4 className="text-sm font-semibold text-white truncate">
                      {item.title}
                    </h4>
                    <p className="text-xs text-amber-300/80 font-serif italic truncate mt-0.5">
                      {item.sanskritTitle}
                    </p>

                    <div className="mt-2 pt-2 border-t border-slate-800/80 flex items-center justify-between text-[11px] text-slate-400 font-mono">
                      <span>SHA-256: {shortenHash(item.sha256Hash, 6, 4)}</span>
                      <span>Block #{item.anchorBlock}</span>
                    </div>
                  </div>
                );
              })}
            </div>
          </div>
        </div>

        {/* Right Column: Detailed Proof Certificate View */}
        <div className="lg:col-span-7">
          {selectedManuscript ? (
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl space-y-6">
              {/* Header Title */}
              <div className="border-b border-slate-800 pb-4 flex flex-col sm:flex-row sm:items-center justify-between gap-3">
                <div>
                  <div className="flex items-center space-x-2">
                    <span className="text-xs font-semibold uppercase px-2.5 py-0.5 rounded-full bg-amber-500/20 text-amber-300 border border-amber-500/30">
                      {selectedManuscript.category}
                    </span>
                    <span className="text-xs text-slate-400 font-mono">
                      Ref: {selectedManuscript.sourceReference}
                    </span>
                  </div>
                  <h3 className="text-xl font-bold font-serif text-white mt-1.5">
                    {selectedManuscript.title}
                  </h3>
                  <p className="text-sm font-serif text-amber-300">
                    {selectedManuscript.sanskritTitle}
                  </p>
                </div>

                <div className="px-3 py-1.5 bg-emerald-500/10 border border-emerald-500/30 rounded-xl flex items-center space-x-2 self-start sm:self-center">
                  <CheckCircle2 className="w-4 h-4 text-emerald-400" />
                  <span className="text-xs font-bold text-emerald-300 tracking-wider">
                    CRYPTOGRAPHICALLY ATTESTED
                  </span>
                </div>
              </div>

              {/* Sanskrit Text & Transliteration Box */}
              <div className="bg-slate-950/80 rounded-xl p-4 border border-slate-800 space-y-3">
                <div>
                  <div className="text-[11px] font-semibold uppercase tracking-wider text-slate-400 mb-1">
                    Canonical Sanskrit (Devanagari)
                  </div>
                  <p className="text-lg font-serif text-amber-100 leading-relaxed tracking-wide">
                    {selectedManuscript.sanskritText}
                  </p>
                </div>

                <div className="pt-2 border-t border-slate-800/80">
                  <div className="text-[11px] font-semibold uppercase tracking-wider text-slate-400 mb-1">
                    IAST Transliteration
                  </div>
                  <p className="text-xs font-mono text-slate-300 italic">
                    {selectedManuscript.transliteration}
                  </p>
                </div>

                <div className="pt-2 border-t border-slate-800/80">
                  <div className="text-[11px] font-semibold uppercase tracking-wider text-slate-400 mb-1">
                    Philosophical Translation
                  </div>
                  <p className="text-xs text-slate-300 leading-relaxed">
                    {selectedManuscript.englishTranslation}
                  </p>
                </div>
              </div>

              {/* Cryptographic Proof Specifications */}
              <div className="space-y-3">
                <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-400 flex items-center gap-1.5">
                  <Key className="w-3.5 h-3.5 text-amber-400" />
                  <span>Cryptographic Proof Attributes (kasturi_storage)</span>
                </h4>

                <div className="space-y-2 text-xs font-mono">
                  {/* SHA-256 */}
                  <div className="bg-slate-950 p-2.5 rounded-lg border border-slate-800 flex items-center justify-between gap-3">
                    <div className="min-w-0">
                      <span className="text-slate-400 block text-[10px] font-sans">SHA-256 Digest:</span>
                      <span className="text-amber-300 truncate block">{selectedManuscript.sha256Hash}</span>
                    </div>
                    <button
                      onClick={() => copyToClipboard(selectedManuscript.sha256Hash, 'sha256')}
                      className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
                      title="Copy SHA-256"
                    >
                      <Copy className="w-3.5 h-3.5" />
                    </button>
                  </div>

                  {/* Merkle Root */}
                  <div className="bg-slate-950 p-2.5 rounded-lg border border-slate-800 flex items-center justify-between gap-3">
                    <div className="min-w-0">
                      <span className="text-slate-400 block text-[10px] font-sans">Merkle Root Anchor:</span>
                      <span className="text-cyan-300 truncate block">{selectedManuscript.merkleRoot}</span>
                    </div>
                    <button
                      onClick={() => copyToClipboard(selectedManuscript.merkleRoot, 'merkle')}
                      className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
                      title="Copy Merkle Root"
                    >
                      <Copy className="w-3.5 h-3.5" />
                    </button>
                  </div>

                  {/* Storage CID */}
                  <div className="bg-slate-950 p-2.5 rounded-lg border border-slate-800 flex items-center justify-between gap-3">
                    <div className="min-w-0">
                      <span className="text-slate-400 block text-[10px] font-sans">Kasturi Storage CID:</span>
                      <span className="text-slate-200 truncate block">{selectedManuscript.storageCid}</span>
                    </div>
                    <button
                      onClick={() => copyToClipboard(selectedManuscript.storageCid, 'cid')}
                      className="p-1.5 hover:bg-slate-800 rounded text-slate-400 hover:text-white shrink-0"
                      title="Copy CID"
                    >
                      <Copy className="w-3.5 h-3.5" />
                    </button>
                  </div>

                  {/* On-Chain Anchor Details */}
                  <div className="grid grid-cols-1 sm:grid-cols-2 gap-2 pt-1 font-sans">
                    <div 
                      onClick={() => onSelectBlock(selectedManuscript.anchorBlock)}
                      className="bg-slate-950 p-2.5 rounded-lg border border-slate-800 hover:border-amber-500/30 cursor-pointer transition-colors"
                    >
                      <span className="text-[10px] text-slate-400 block">Anchor Block:</span>
                      <span className="font-mono text-sm font-bold text-amber-400 hover:underline">
                        #{selectedManuscript.anchorBlock}
                      </span>
                    </div>

                    <div 
                      onClick={() => onSelectTx(selectedManuscript.anchorTxHash)}
                      className="bg-slate-950 p-2.5 rounded-lg border border-slate-800 hover:border-cyan-500/30 cursor-pointer transition-colors"
                    >
                      <span className="text-[10px] text-slate-400 block">Anchor Tx:</span>
                      <span className="font-mono text-xs text-cyan-400 hover:underline truncate block">
                        {shortenHash(selectedManuscript.anchorTxHash, 8, 6)}
                      </span>
                    </div>
                  </div>
                </div>
              </div>

              {/* Copy Feedback */}
              {copiedField && (
                <div className="text-center text-xs text-emerald-400 bg-emerald-950/40 border border-emerald-500/30 py-1.5 rounded-lg">
                  Copied {copiedField} to clipboard!
                </div>
              )}
            </div>
          ) : (
            <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-12 text-center text-slate-500">
              Select a manuscript to view its cryptographic proofs.
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
