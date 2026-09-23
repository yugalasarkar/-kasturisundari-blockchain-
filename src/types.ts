export interface NetworkMetrics {
  blockNumber: number;
  blockNumberHex: string;
  chainId: number;
  chainIdHex: string;
  gasPriceGwei: number;
  gasPriceWei: string;
  latencyMs: number;
  isLive: boolean;
  rpcEndpoint: string;
  lastUpdated: Date;
}

export interface BlockData {
  number: number;
  numberHex: string;
  hash: string;
  parentHash: string;
  nonce?: string;
  sha3Uncles?: string;
  logsBloom?: string;
  transactionsRoot?: string;
  stateRoot?: string;
  receiptsRoot?: string;
  miner: string;
  difficulty?: string;
  totalDifficulty?: string;
  extraData?: string;
  size?: number;
  gasLimit: number;
  gasUsed: number;
  timestamp: number;
  transactions: (string | TransactionData)[];
  baseFeePerGas?: string;
}

export interface TransactionData {
  hash: string;
  nonce: number;
  blockHash: string;
  blockNumber: number;
  transactionIndex: number;
  from: string;
  to: string | null;
  valueKST: string;
  valueWei: string;
  gasPriceGwei: string;
  gasPriceWei: string;
  gas: number;
  gasUsed?: number;
  input: string;
  status?: 'success' | 'failed' | 'pending';
  receipt?: TransactionReceipt;
  timestamp?: number;
}

export interface TransactionReceipt {
  transactionHash: string;
  transactionIndex: number;
  blockHash: string;
  blockNumber: number;
  from: string;
  to: string | null;
  cumulativeGasUsed: number;
  gasUsed: number;
  contractAddress: string | null;
  status: boolean; // true = success, false = failure
  logsBloom?: string;
  effectiveGasPrice?: string;
}

export interface AccountData {
  address: string;
  balanceKST: string;
  balanceWei: string;
  transactionCount: number;
  isContract: boolean;
  bytecode?: string;
  storageHash?: string;
}

export interface ManuscriptRecord {
  id: string;
  title: string;
  sanskritTitle: string;
  category: 'Shruti (Veda)' | 'Smriti' | 'Itihasa / Gita' | 'Jyotisha / Ganita' | 'Tantra / Agama' | 'Upanishad';
  sourceReference: string;
  sanskritText: string;
  transliteration: string;
  englishTranslation: string;
  sha256Hash: string;
  keccak256Hash: string;
  merkleRoot: string;
  storageCid: string;
  anchorTxHash: string;
  anchorBlock: number;
  timestamp: number;
  verifiedStatus: 'VERIFIED' | 'ANCHORED';
  witnessValidator: string;
}
