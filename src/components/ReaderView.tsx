import React, { useState } from 'react';
import { Grantha, Khanda, Adhyaya, Verse } from '../types';
import { 
  ArrowLeft, 
  ArrowRight, 
  Copy, 
  Check, 
  Volume2, 
  VolumeX, 
  Type, 
  Bookmark
} from 'lucide-react';
import { 
  normalizeShastricPunctuation, 
  toDevanagariNumeral 
} from '../utils/shastraFormatter';

interface ReaderViewProps {
  grantha: Grantha;
  khanda: Khanda;
  adhyayaId: number;
  totalAdhyayas: number;
  adhyayasList: Adhyaya[];
  verses: Verse[];
  onSelectAdhyaya: (id: number) => void;
  onBackToKhanda: () => void;
  isDronePlaying: boolean;
  onToggleDrone: () => void;
}

// संस्कृत-क्रमबोधक-नामानि (अध्यायाः १-३०)
const SANSKRIT_ORDINALS: Record<number, string> = {
  1: 'प्रथमोऽध्यायः',
  2: 'द्वितीयोऽध्यायः',
  3: 'तृतीयोऽध्यायः',
  4: 'चतुर्थोऽध्यायः',
  5: 'पञ्चमोऽध्यायः',
  6: 'षष्ठोऽध्यायः',
  7: 'सप्तमोऽध्यायः',
  8: 'अष्टमोऽध्यायः',
  9: 'नवमोऽध्यायः',
  10: 'दशमोऽध्यायः',
  11: 'एकादशोऽध्यायः',
  12: 'द्वादशोऽध्यायः',
  13: 'त्रयोदशोऽध्यायः',
  14: 'चतुर्दशोऽध्यायः',
  15: 'पञ्चदशोऽध्यायः',
  16: 'षोडशोऽध्यायः',
  17: 'सप्तदशोऽध्यायः',
  18: 'अष्टादशोऽध्यायः',
  19: 'एकोनविंशोऽध्यायः',
  20: 'विंशोऽध्यायः',
  21: 'एकविंशोऽध्यायः',
  22: 'द्वाविंशोऽध्यायः',
  23: 'त्रयोविंशोऽध्यायः',
  24: 'चतुर्विंशोऽध्यायः',
  25: 'पञ्चविंशोऽध्यायः',
  26: 'षड्विंशोऽध्यायः'
};

export const ReaderView: React.FC<ReaderViewProps> = ({
  grantha,
  khanda,
  adhyayaId,
  totalAdhyayas,
  adhyayasList,
  verses,
  onSelectAdhyaya,
  onBackToKhanda,
  isDronePlaying,
  onToggleDrone
}) => {
  const [copiedIndex, setCopiedIndex] = useState<number | null>(null);
  const [fontSizeLevel, setFontSizeLevel] = useState<'normal' | 'large' | 'xlarge'>('normal');
  const [savedBookmarks, setSavedBookmarks] = useState<number[]>([]);

  const currentAdhyaya = adhyayasList.find((a) => a.id === adhyayaId);
  const chapterOrdinal = SANSKRIT_ORDINALS[adhyayaId] || `अध्यायः ${adhyayaId}`;

  const handleCopyVerse = (v: Verse, index: number) => {
    const normShloka = normalizeShastricPunctuation(v.shloka, v.num);
    const devNum = toDevanagariNumeral(v.num);
    const textToCopy = `॥ श्लोकः ${devNum} ॥\n${normShloka}\n\nअन्वयः : ${v.anvaya}\n\n— ${grantha.title}, ${khanda.name} (${toDevanagariNumeral(khanda.id)}.${toDevanagariNumeral(adhyayaId)}.${devNum})`;
    navigator.clipboard.writeText(textToCopy);
    setCopiedIndex(index);
    setTimeout(() => setCopiedIndex(null), 2000);
  };

  const toggleBookmark = (verseNum: number) => {
    setSavedBookmarks((prev) =>
      prev.includes(verseNum) ? prev.filter((n) => n !== verseNum) : [...prev, verseNum]
    );
  };

  const getShlokaSizeClass = () => {
    switch (fontSizeLevel) {
      case 'large':
        return 'text-xl sm:text-2xl leading-[2.2]';
      case 'xlarge':
        return 'text-2xl sm:text-3xl leading-[2.4]';
      default:
        return 'text-lg sm:text-xl leading-loose';
    }
  };

  return (
    <div className="max-w-3xl mx-auto space-y-6 animate-fadeIn pb-12">
      {/* १. शीर्षमार्गदर्शनम् (Breadcrumb & Navigation) */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-4 border-b border-[#21232d] gap-3">
        <button 
          id="reader-back-btn"
          onClick={onBackToKhanda}
          className="text-xs text-stone-300 hover:text-[#c5a059] flex items-center gap-1.5 transition-colors cursor-pointer font-serif"
        >
          <ArrowLeft className="w-3.5 h-3.5 text-[#c5a059]" /> 
          <span>{khanda.name} (अध्यायसूची)</span>
        </button>

        <div className="flex items-center gap-3 text-xs font-serif text-stone-300">
          <span>
            {grantha.title} › {khanda.name} › <span className="text-[#e8be6b]">अध्यायः {adhyayaId}</span>
          </span>
          <span className="hidden sm:inline text-stone-600">|</span>
          <select
            id="chapter-quick-jump"
            value={adhyayaId}
            onChange={(e) => onSelectAdhyaya(Number(e.target.value))}
            className="bg-[#14151b] text-stone-200 border border-[#262834] rounded px-2.5 py-1 text-xs font-serif cursor-pointer outline-none focus:border-[#c5a059]"
          >
            {adhyayasList.map((a) => (
              <option key={a.id} value={a.id}>
                {a.title} {a.id === 2 && khanda.id === 4 ? '(मन्दिरसेवा)' : ''}
              </option>
            ))}
          </select>
        </div>
      </div>

      {/* २. श्लोकवाचन-नियन्त्रणफलकम् (Font size, Tanpura drone) */}
      <div className="flex flex-wrap items-center justify-between gap-2 p-2.5 bg-[#12131a] rounded-lg border border-[#21232d] text-xs">
        <div className="flex items-center gap-1.5">
          <span className="text-stone-400 font-serif text-xs flex items-center gap-1">
            <Type className="w-3 h-3 text-[#c5a059]" /> अक्षरपरिमाणम् :
          </span>
          <button
            onClick={() => setFontSizeLevel('normal')}
            className={`px-2.5 py-0.5 rounded font-serif text-xs cursor-pointer transition-all ${
              fontSizeLevel === 'normal'
                ? 'bg-[#c5a059]/20 text-[#e8be6b] font-bold border border-[#c5a059]/40'
                : 'text-stone-400 hover:text-stone-200'
            }`}
          >
            ह्रस्व
          </button>
          <button
            onClick={() => setFontSizeLevel('large')}
            className={`px-2.5 py-0.5 rounded font-serif text-xs cursor-pointer transition-all ${
              fontSizeLevel === 'large'
                ? 'bg-[#c5a059]/20 text-[#e8be6b] font-bold border border-[#c5a059]/40'
                : 'text-stone-400 hover:text-stone-200'
            }`}
          >
            मध्यम
          </button>
          <button
            onClick={() => setFontSizeLevel('xlarge')}
            className={`px-2.5 py-0.5 rounded font-serif text-xs cursor-pointer transition-all ${
              fontSizeLevel === 'xlarge'
                ? 'bg-[#c5a059]/20 text-[#e8be6b] font-bold border border-[#c5a059]/40'
                : 'text-stone-400 hover:text-stone-200'
            }`}
          >
            दीर्घ
          </button>
        </div>

        <div className="flex items-center gap-2">
          {/* Tanpura drone button */}
          <button
            onClick={onToggleDrone}
            className={`px-3 py-1 rounded font-serif text-xs flex items-center gap-1.5 border cursor-pointer transition-all ${
              isDronePlaying
                ? 'bg-[#c5a059]/20 text-[#e8be6b] border-[#c5a059]'
                : 'bg-[#171821] text-stone-300 border-[#282a36] hover:text-stone-100 hover:border-[#c5a059]/40'
            }`}
          >
            {isDronePlaying ? <Volume2 className="w-3.5 h-3.5 text-[#e8be6b]" /> : <VolumeX className="w-3.5 h-3.5 text-stone-500" />}
            <span>{isDronePlaying ? 'तानपूरा (सक्रियम्)' : 'तानपूरा नादः'}</span>
          </button>
        </div>
      </div>

      {/* ३. अध्याय-शीर्षकम् (Chapter Title Header) */}
      <div className="text-center py-6 space-y-2 bg-[#101117] rounded-xl border border-[#1f212a] px-4 shadow-sm relative overflow-hidden">
        <div className="text-xs font-serif text-[#c5a059] bg-[#c5a059]/10 inline-block px-3.5 py-0.5 rounded-full border border-[#c5a059]/20 mb-1">
          {grantha.title} • {khanda.name}
        </div>
        <h2 className="text-2xl sm:text-3xl font-bold text-[#e8be6b] font-serif tracking-wide">
          अथ {chapterOrdinal}
        </h2>
        <p className="text-xs sm:text-sm text-stone-300 font-serif max-w-xl mx-auto leading-relaxed">
          {currentAdhyaya?.summary || 'भगवन्माहात्म्य-पुण्यकथा-प्रसङ्गः'}
        </p>
        <div className="text-xs font-serif text-stone-400 pt-1">
          {verses.length} श्लोकाः
        </div>
      </div>

      {/* ४. श्लोकमाला (Shloka Stream with Strict Shastric Prosody & Pure Sanskrit Anvaya) */}
      <div className="space-y-6">
        {verses.map((v, index) => {
          const isBookmarked = savedBookmarks.includes(v.num);
          const normalizedShloka = normalizeShastricPunctuation(v.shloka, v.num);
          const devNum = toDevanagariNumeral(v.num);

          return (
            <div 
              key={v.num} 
              id={`verse-card-${v.num}`}
              className="p-6 sm:p-7 rounded-xl bg-[#111218] border border-[#21232d] hover:border-[#333647] transition-all space-y-4 shadow-md group relative"
            >
              {/* Verse metadata & action buttons */}
              <div className="flex justify-between items-center text-xs font-serif text-stone-400 pb-2 border-b border-[#1b1c24]">
                <div className="flex items-center gap-2">
                  <span className="text-[#c5a059] font-bold font-serif text-sm">
                    ॥ श्लोकः {devNum} ॥
                  </span>
                  {isBookmarked && (
                    <span className="text-[10px] bg-[#c5a059]/20 text-[#e8be6b] px-2 py-0.5 rounded font-serif">
                      सुरक्षितः
                    </span>
                  )}
                </div>

                <div className="flex items-center gap-3">
                  <span className="text-xs font-serif text-stone-400">
                    {toDevanagariNumeral(khanda.id)}.{toDevanagariNumeral(adhyayaId)}.{devNum}
                  </span>

                  {/* Bookmark action */}
                  <button
                    onClick={() => toggleBookmark(v.num)}
                    className="text-stone-400 hover:text-[#c5a059] p-1 cursor-pointer transition-colors"
                  >
                    <Bookmark className={`w-3.5 h-3.5 ${isBookmarked ? 'fill-[#c5a059] text-[#c5a059]' : ''}`} />
                  </button>

                  {/* Copy verse */}
                  <button
                    onClick={() => handleCopyVerse(v, index)}
                    className="text-stone-400 hover:text-[#c5a059] p-1 cursor-pointer transition-colors flex items-center gap-1"
                  >
                    {copiedIndex === index ? (
                      <>
                        <Check className="w-3.5 h-3.5 text-emerald-400" />
                        <span className="text-xs text-emerald-400 font-serif">प्रतिलिपिः कृता</span>
                      </>
                    ) : (
                      <Copy className="w-3.5 h-3.5" />
                    )}
                  </button>
                </div>
              </div>
              
              {/* Devanagari Shloka Text (Strict Shastric Danda & Verse Numeral) */}
              <div 
                className={`${getShlokaSizeClass()} font-serif text-stone-100 whitespace-pre-line text-center py-4 px-2 tracking-wide font-medium select-text`}
              >
                {normalizedShloka}
              </div>

              {/* Anvaya (अन्वयः पदच्छेद-क्रमशः) */}
              <div className="pt-3 border-t border-[#1d1f28] text-xs font-serif leading-relaxed bg-[#0d0e13]/60 p-3.5 rounded-lg">
                <span className="text-[#c5a059] text-xs block mb-1 font-semibold flex items-center gap-1.5">
                  <span className="w-1.5 h-1.5 rounded-full bg-[#c5a059]" />
                  अन्वयः (पदच्छेद-क्रमशः) :
                </span>
                <div className="pl-3 font-serif text-stone-200 text-[13px] leading-relaxed">
                  {v.anvaya}
                </div>
              </div>
            </div>
          );
        })}
      </div>

      {/* ५. अध्याय-परिवर्तनम् (Chapter Navigation Footer) */}
      <div className="flex flex-col sm:flex-row items-center justify-between pt-8 border-t border-[#21232d] gap-3">
        <button 
          id="prev-chapter-btn"
          disabled={adhyayaId <= 1}
          onClick={() => {
            onSelectAdhyaya(Math.max(1, adhyayaId - 1));
          }}
          className={`w-full sm:w-auto px-4 py-2.5 rounded-lg border text-xs flex items-center justify-center gap-2 transition-all cursor-pointer font-serif ${
            adhyayaId <= 1
              ? 'opacity-40 cursor-not-allowed bg-[#111218] border-[#1d1f26] text-stone-600'
              : 'bg-[#14151b] border-[#23242c] hover:border-[#c5a059] hover:bg-[#181a24] text-stone-300'
          }`}
        >
          <ArrowLeft className="w-3.5 h-3.5 text-[#c5a059]" /> 
          <span>पूर्वोऽध्यायः</span>
        </button>

        <div className="text-xs font-serif text-stone-300">
          अध्यायः {toDevanagariNumeral(adhyayaId)} / {toDevanagariNumeral(totalAdhyayas)}
        </div>

        <button 
          id="next-chapter-btn"
          disabled={adhyayaId >= totalAdhyayas}
          onClick={() => {
            onSelectAdhyaya(Math.min(totalAdhyayas, adhyayaId + 1));
          }}
          className={`w-full sm:w-auto px-4 py-2.5 rounded-lg border text-xs flex items-center justify-center gap-2 transition-all cursor-pointer font-serif ${
            adhyayaId >= totalAdhyayas
              ? 'opacity-40 cursor-not-allowed bg-[#111218] border-[#1d1f26] text-stone-600'
              : 'bg-[#14151b] border-[#23242c] hover:border-[#c5a059] hover:bg-[#181a24] text-[#c5a059] font-medium'
          }`}
        >
          <span>अग्रिमोऽध्यायः</span>
          <ArrowRight className="w-3.5 h-3.5" />
        </button>
      </div>
    </div>
  );
};
