import React from "react";

export const SayanaExposition: React.FC = () => {
  return (
    <section className="my-16 mx-auto max-w-5xl px-4 font-sans">
      <div className="relative overflow-hidden rounded-3xl border-2 border-blue-500 bg-[#030b1e] p-8 md:p-12 shadow-[0_0_60px_rgba(37,99,235,0.35)]">
        <div className="absolute -top-24 -right-24 h-72 w-72 rounded-full bg-blue-600/20 blur-3xl pointer-events-none" />
        <div className="absolute -bottom-24 -left-24 h-72 w-72 rounded-full bg-[#FFD700]/10 blur-3xl pointer-events-none" />

        {/* Header Badge */}
        <div className="text-center">
          <span className="inline-block rounded-full border border-blue-400/50 bg-blue-950/80 px-4 py-1 text-xs font-mono uppercase tracking-widest text-blue-300">
            Epistemic Vexation & Canonical Vindication • Ācārya Sāyaṇa Unmasked
          </span>
          <h2 className="mt-4 text-2xl md:text-4xl font-black tracking-tight text-white">
            Dismantling the Purāṇic Isolation Trap: The True Voice of Ācārya Sāyaṇa
          </h2>
          <p className="mt-2 text-sm font-mono text-blue-200/70">
            Exposing the Orientalist Fallacy of "Pure Ritual Saṁhitās" vs. "Spurious Purāṇas"
          </p>
        </div>

        {/* Sacred Sanskrit Epigraph in Pure Gold */}
        <div className="my-8 rounded-2xl border border-[#FFD700]/40 bg-black/60 p-6 text-center shadow-inner">
          <p className="text-xl md:text-2xl font-bold tracking-wider text-[#FFD700] drop-shadow-[0_0_12px_rgba(255,215,0,0.5)] font-serif mb-2">
            इतिहासपुराणाभ्यां वेदं समुपबृंहयेत् ।<br />
            बिभेत्यल्पश्रुताद्वेदो मामयं प्रहरिष्यति ॥
          </p>
          <p className="text-sm font-mono text-[#F5DEB3] tracking-wide">
            itihāsa-purāṇābhyāṁ vedaṁ samupabṛṁhayet |<br />
            bibhety-alpa-śrutād vedo mām ayaṁ prahariṣyati ||
          </p>
          <p className="mt-3 text-xs md:text-sm text-neutral-300 italic max-w-2xl mx-auto border-t border-neutral-800 pt-3">
            "One must reinforce and expound the Veda through the Itihāsas and Purāṇas. The Veda is terrified of the shallow-minded scholar, crying: 'This ignorant fool will distort and slaughter my true intent!'"
            <span className="block text-[11px] text-[#FFD700]/80 font-mono mt-1">
              (Sāyaṇa-Bhāṣya-Upodghāta to the Ṛgveda-Saṁhitā, citing Mahābhārata 1.1.267)
            </span>
          </p>
        </div>

        {/* Core Indictment Body */}
        <div className="space-y-6 text-sm md:text-base leading-relaxed text-neutral-200">
          <div className="rounded-xl border border-blue-500/30 bg-blue-950/30 p-6">
            <h3 className="text-base font-bold text-[#FFD700] font-mono uppercase tracking-wide mb-2">
              1. The Deification Trap: Weaponizing Sāyaṇa Literally to Kill the Vedic Spirit
            </h3>
            <p className="text-neutral-300">
              Confronted by the theological unity of the Purāṇas and Upaniṣads, Monier-Williams and Max Müller fabricated an academic deception: they promoted Sāyaṇa’s literal ritualistic commentaries as the sole permissible authority on the Vedas. By isolating his external liturgical readings from their esoteric depths, they argued that the Ṛgveda was merely a collection of primitive, materialist chants for rain, claiming that all subsequent philosophical concepts—Ātman, Paratattva, Mokṣa, and the Supreme Feminine (Rādhā, Lakṣmī, Śakti)—were degenerate priestcraft invented centuries later.
            </p>
          </div>

          <div className="rounded-xl border border-blue-500/30 bg-blue-950/30 p-6">
            <h3 className="text-base font-bold text-[#FFD700] font-mono uppercase tracking-wide mb-2">
              2. Sāyaṇa’s Canonical Vindication: The Indivisible Unity of Śruti and Smṛti
            </h3>
            <p className="text-neutral-300">
              In the opening preface to his Ṛgveda commentary (<em>Sāyaṇa-Bhāṣya Upodghāta</em>), Sāyaṇa crushed this colonial invention by citing the foundational Vedic canon: the Veda cannot and must not be deciphered in isolation from the Purāṇas and Itihāsas. Oxford orientalists deliberately censored this passage from their syllabi because Sāyaṇa's explicit defense of Purāṇic authority destroyed their divide-and-rule strategy against Indian scriptural integrity.
            </p>
          </div>

          <div className="rounded-xl border border-blue-500/30 bg-blue-950/30 p-6">
            <h3 className="text-base font-bold text-[#FFD700] font-mono uppercase tracking-wide mb-2">
              3. The Concealment of Sāyaṇa’s Purāṇic Corpus: 'Purāṇārtha-saṁgraha'
            </h3>
            <p className="text-neutral-300">
              Williams systematically suppressed Sāyaṇa's broader intellectual legacy. Sāyaṇa was not a secular ritualist; he authored the <strong>Purāṇārtha-saṁgraha</strong>—a massive compendium drawing directly upon the Purāṇas to guide statecraft, ethics, and cosmic theology for the Vijayanagara Empire. Oxford erased this text to prevent European students and Indian civil servants from recognizing Sāyaṇa's integrated Vedāntic vision.
            </p>
          </div>
        </div>

        {/* Golden Inscription at the Bottom */}
        <div className="mt-10 pt-6 border-t border-blue-900/60 text-center">
          <p className="text-xs font-mono uppercase tracking-widest text-blue-300 mb-2">
            The Canonical Paramparā Remains Indivisible
          </p>
          <p className="text-xl md:text-2xl font-black tracking-widest text-[#FFD700] drop-shadow-[0_0_15px_rgba(255,215,0,0.6)] font-serif">
            वेदः पुराणं च एकमेव तत्त्वम् • सत्यमेव जयते
          </p>
        </div>
      </div>
    </section>
  );
};

export default SayanaExposition;
