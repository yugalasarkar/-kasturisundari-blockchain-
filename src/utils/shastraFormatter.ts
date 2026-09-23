/**
 * Shastric Punctuation & Verse Numerals Normalizer
 * Enforces authentic Sanskrit metrics (Danda rules) across all rendered verses:
 * - End of first half (Pādas a & b): single Danda ( ।)
 * - End of second half (Pādas c & d): double Danda with Devanagari verse numeral ( ॥ १ ॥)
 * - Avoids double Dandas at the end of both lines.
 */

const DEVANAGARI_DIGITS = ['०', '१', '२', '३', '४', '५', '६', '७', '८', '९'];

export function toDevanagariNumeral(num: number): string {
  return num
    .toString()
    .split('')
    .map((digit) => DEVANAGARI_DIGITS[parseInt(digit, 10)] ?? digit)
    .join('');
}

/**
 * Normalizes any raw Shloka string to strictly adhere to standard Shastric metric punctuation:
 * Example output:
 * अथातः प्रवक्ष्यामि शृणुध्वं मुनिपुङ्गवाः ।
 * यत्कीर्तनात्प्रमुच्येत सर्वपापैर्न संशयः ॥ १ ॥
 */
export function normalizeShastricPunctuation(rawShloka: string, verseNum: number): string {
  if (!rawShloka) return '';

  const lines = rawShloka
    .split('\n')
    .map((line) => line.trim())
    .filter(Boolean);

  if (lines.length === 0) return '';

  const devNum = toDevanagariNumeral(verseNum);

  // Helper to clean trailing dandas, numbers, and trailing whitespace from a line
  const cleanLineEnding = (line: string): string => {
    return line.replace(/[\s।॥|0-9०-९\-—]+$/, '').trim();
  };

  // Standard 2-line Shloka
  if (lines.length === 2) {
    const firstHalf = cleanLineEnding(lines[0]);
    const secondHalf = cleanLineEnding(lines[1]);
    return `${firstHalf} ।\n${secondHalf} ॥ ${devNum} ॥`;
  }

  // 3-line verse with introductory speaker or invocation (e.g. "सूत उवाच —" or "ॐ...")
  if (lines.length === 3) {
    const introRaw = lines[0].replace(/[\s।॥|]+$/, '').trim();
    const isSpeaker = /(?:उवाच|ऊचुः|—|:)/.test(lines[0]);
    const intro = isSpeaker
      ? introRaw.endsWith('—') ? introRaw : `${introRaw} —`
      : `${introRaw} ।`;

    const firstHalf = cleanLineEnding(lines[1]);
    const secondHalf = cleanLineEnding(lines[2]);
    return `${intro}\n${firstHalf} ।\n${secondHalf} ॥ ${devNum} ॥`;
  }

  // Single line / ardhashloka
  if (lines.length === 1) {
    const half = cleanLineEnding(lines[0]);
    return `${half} ॥ ${devNum} ॥`;
  }

  // Fallback for > 3 lines: format the last 2 lines as the metered shloka
  const introLines = lines.slice(0, lines.length - 2);
  const firstHalf = cleanLineEnding(lines[lines.length - 2]);
  const secondHalf = cleanLineEnding(lines[lines.length - 1]);

  return [
    ...introLines,
    `${firstHalf} ।`,
    `${secondHalf} ॥ ${devNum} ॥`
  ].join('\n');
}

/**
 * Normalizes IAST transliteration to match the shastric metrics:
 * Line 1 ends with single pipe ( |)
 * Line 2 ends with double pipe and verse numeral ( || 1 ||)
 */
export function normalizeTranslitPunctuation(rawTranslit: string, verseNum: number): string {
  if (!rawTranslit) return '';

  const lines = rawTranslit
    .split('\n')
    .map((line) => line.trim())
    .filter(Boolean);

  if (lines.length === 0) return '';

  const cleanLineEnding = (line: string): string => {
    return line.replace(/[\s|।॥0-9\-—]+$/, '').trim();
  };

  if (lines.length === 2) {
    const firstHalf = cleanLineEnding(lines[0]);
    const secondHalf = cleanLineEnding(lines[1]);
    return `${firstHalf} |\n${secondHalf} || ${verseNum} ||`;
  }

  if (lines.length === 3) {
    const introRaw = lines[0].replace(/[\s|]+$/, '').trim();
    const isSpeaker = /(?:uvāca|ūcuḥ|—|:)/i.test(lines[0]);
    const intro = isSpeaker
      ? introRaw.endsWith('—') ? introRaw : `${introRaw} —`
      : `${introRaw} |`;

    const firstHalf = cleanLineEnding(lines[1]);
    const secondHalf = cleanLineEnding(lines[2]);
    return `${intro}\n${firstHalf} |\n${secondHalf} || ${verseNum} ||`;
  }

  if (lines.length === 1) {
    const half = cleanLineEnding(lines[0]);
    return `${half} || ${verseNum} ||`;
  }

  const introLines = lines.slice(0, lines.length - 2);
  const firstHalf = cleanLineEnding(lines[lines.length - 2]);
  const secondHalf = cleanLineEnding(lines[lines.length - 1]);

  return [
    ...introLines,
    `${firstHalf} |`,
    `${secondHalf} || ${verseNum} ||`
  ].join('\n');
}
