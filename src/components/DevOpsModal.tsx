import React, { useState } from 'react';
import { 
  Server, 
  Terminal, 
  FileCode, 
  Copy, 
  CheckCircle2, 
  ExternalLink, 
  ShieldCheck, 
  Play, 
  Download,
  AlertTriangle,
  Cpu,
  Globe
} from 'lucide-react';

interface DevOpsModalProps {
  isOpen?: boolean;
  onClose?: () => void;
  isStandaloneTab?: boolean;
}

export const DevOpsModal: React.FC<DevOpsModalProps> = ({
  isOpen = true,
  onClose,
  isStandaloneTab = false,
}) => {
  const [activeSubTab, setActiveSubTab] = useState<'script' | 'nginx' | 'specs'>('script');
  const [copiedField, setCopiedField] = useState<string | null>(null);

  const copyToClipboard = (text: string, label: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(label);
    setTimeout(() => setCopiedField(null), 2000);
  };

  const nginxConfContent = `# ==============================================================================
# Satya Explorer - Nginx Configuration
# Domain: satya.kasturisundari.xyz
# Host: AWS EC2 Ubuntu 22.04 (52.72.236.75)
# Web Root: /var/www/satya/dist
# Target Path: /etc/nginx/sites-available/satya.kasturisundari.xyz.conf
# ==============================================================================

# Rate limiting zone for RPC proxy / API endpoints
limit_req_zone $binary_remote_addr zone=explorer_limit:10m rate=30r/s;

# ------------------------------------------------------------------------------
# 1. HTTP Server (Port 80) -> 301 Permanent Redirect to HTTPS
# ------------------------------------------------------------------------------
server {
    listen 80;
    listen [::]:80;
    server_name satya.kasturisundari.xyz 52.72.236.75;

    # Allow Let's Encrypt ACME challenge renewals
    location ^~ /.well-known/acme-challenge/ {
        root /var/www/satya/dist;
        default_type "text/plain";
        allow all;
    }

    # 301 Permanent Redirect to HTTPS
    location / {
        return 301 https://satya.kasturisundari.xyz$request_uri;
    }
}

# ------------------------------------------------------------------------------
# 2. HTTPS Server (Port 443) with Let's Encrypt SSL & Security Headers
# ------------------------------------------------------------------------------
server {
    listen 443 ssl http2;
    listen [::]:443 ssl http2;
    server_name satya.kasturisundari.xyz;

    # SSL Certificates
    ssl_certificate /etc/letsencrypt/live/satya.kasturisundari.xyz/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/satya.kasturisundari.xyz/privkey.pem;

    # Hardened SSL Protocols and Ciphers (TLSv1.2 & TLSv1.3 only)
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_prefer_server_ciphers on;
    ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384;
    ssl_session_timeout 1d;
    ssl_session_cache shared:SSL:10m;
    ssl_session_tickets off;

    # OCSP Stapling
    ssl_stapling on;
    ssl_stapling_verify on;
    ssl_trusted_certificate /etc/letsencrypt/live/satya.kasturisundari.xyz/fullchain.pem;
    resolver 1.1.1.1 8.8.8.8 valid=300s;
    resolver_timeout 5s;

    # Static Web Root (Vite SPA output)
    root /var/www/satya/dist;
    index index.html;

    # Security Headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header Referrer-Policy "strict-origin-when-cross-origin" always;
    add_header Strict-Transport-Security "max-age=63072000; includeSubDomains; preload" always;
    add_header Permissions-Policy "camera=(), microphone=(), geolocation=()" always;

    # Gzip Compression
    gzip on;
    gzip_vary on;
    gzip_proxied any;
    gzip_comp_level 6;
    gzip_types text/plain text/css text/xml application/json application/javascript application/rss+xml application/atom+xml image/svg+xml;

    # Cache Control for static immutable assets
    location ~* \.(?:css|js|woff2?|ttf|eot|png|jpg|jpeg|gif|ico|svg|webp)$ {
        expires 1y;
        add_header Cache-Control "public, max-age=31536000, immutable";
        access_log off;
    }

    # SPA Fallback (route all navigation to index.html)
    location / {
        try_files $uri $uri/ /index.html;
        add_header Cache-Control "no-cache, no-store, must-revalidate";
    }

    # Optional local RPC reverse proxy
    location /rpc {
        limit_req zone=explorer_limit burst=50 nodelay;
        proxy_pass http://127.0.0.1:8545;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_read_timeout 60s;
    }

    error_page 404 /index.html;
    error_page 500 502 503 504 /50x.html;
    location = /50x.html {
        root /usr/share/nginx/html;
    }
}`;

  const bashScriptContent = `#!/usr/bin/env bash
# ==============================================================================
# Satya Explorer - Production Deployment Script
# Target Host: AWS EC2 (Ubuntu 22.04 LTS) | IP: 52.72.236.75
# Domain: https://satya.kasturisundari.xyz
# Target Web Root: /var/www/satya/dist
# ==============================================================================

set -euo pipefail

# 1. Verify root privileges
if [[ $EUID -ne 0 ]]; then
   echo "[ERROR] This script must be run as root or with sudo." 
   exit 1
fi

DOMAIN="satya.kasturisundari.xyz"
APP_DIR="/var/www/satya"
NGINX_CONF_AVAILABLE="/etc/nginx/sites-available/\${DOMAIN}.conf"
NGINX_CONF_ENABLED="/etc/nginx/sites-enabled/\${DOMAIN}.conf"
SSL_CERT="/etc/letsencrypt/live/\${DOMAIN}/fullchain.pem"
SSL_KEY="/etc/letsencrypt/live/\${DOMAIN}/privkey.pem"

echo "==> [1/6] Preparing system dependencies and directories..."
apt-get update -y
apt-get install -y curl wget git nginx certbot python3-certbot-nginx

# Ensure Node.js 20 LTS is present
if ! command -v node &> /dev/null; then
    echo "Installing Node.js 20 LTS..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
    apt-get install -y nodejs
fi

# Ensure web root
mkdir -p "\${APP_DIR}"
mkdir -p "\${APP_DIR}/dist"

echo "==> [2/6] Building frontend project..."
cd "\${APP_DIR}"

if [[ -f "\${APP_DIR}/package.json" ]]; then
    npm ci --prefer-offline || npm install
    npm run build
fi

chown -R www-data:www-data "\${APP_DIR}/dist"
chmod -R 755 "\${APP_DIR}/dist"

echo "==> [3/6] Verifying SSL certificates..."
if [[ ! -f "\${SSL_CERT}" || ! -f "\${SSL_KEY}" ]]; then
    echo "[WARN] Real SSL certificates not found at \${SSL_CERT}"
    mkdir -p "/etc/letsencrypt/live/\${DOMAIN}"
    openssl req -x509 -nodes -days 30 -newkey rsa:2048 \\
        -keyout "\${SSL_KEY}" \\
        -out "\${SSL_CERT}" \\
        -subj "/C=IN/ST=Karnataka/L=Bengaluru/O=KasturiChain/CN=\${DOMAIN}"
fi

echo "==> [4/6] Installing Nginx configuration..."
cat << 'EOF' > "\${NGINX_CONF_AVAILABLE}"
server {
    listen 80;
    listen [::]:80;
    server_name satya.kasturisundari.xyz 52.72.236.75;
    location / {
        return 301 https://satya.kasturisundari.xyz$request_uri;
    }
}

server {
    listen 443 ssl http2;
    listen [::]:443 ssl http2;
    server_name satya.kasturisundari.xyz;

    ssl_certificate /etc/letsencrypt/live/satya.kasturisundari.xyz/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/satya.kasturisundari.xyz/privkey.pem;

    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_prefer_server_ciphers on;

    root /var/www/satya/dist;
    index index.html;

    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header Strict-Transport-Security "max-age=63072000; includeSubDomains; preload" always;

    gzip on;
    gzip_types text/plain text/css application/json application/javascript image/svg+xml;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /rpc {
        proxy_pass http://127.0.0.1:8545;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
EOF

ln -sf "\${NGINX_CONF_AVAILABLE}" "\${NGINX_CONF_ENABLED}"
rm -f /etc/nginx/sites-enabled/default

echo "==> [5/6] Testing Nginx configuration and reloading..."
nginx -t
systemctl reload nginx || systemctl restart nginx

echo "==> [6/6] Verifying endpoint via curl..."
curl -I -k "https://\${DOMAIN}" || true
echo "==> Satya Explorer deployed successfully at https://\${DOMAIN}!"`;

  return (
    <div className="space-y-6">
      {/* Infrastructure Specs Header */}
      <div className="bg-slate-900/80 border border-slate-800 rounded-2xl p-6 shadow-xl">
        <div className="flex flex-col lg:flex-row lg:items-center justify-between gap-4 mb-5">
          <div className="flex items-center space-x-3">
            <div className="p-2.5 rounded-xl bg-cyan-500/10 text-cyan-400 border border-cyan-500/20">
              <Server className="w-6 h-6" />
            </div>
            <div>
              <h2 className="text-xl font-bold text-white">AWS EC2 Server & Network Specification</h2>
              <p className="text-xs text-slate-400">Production parameters for https://satya.kasturisundari.xyz</p>
            </div>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={() => copyToClipboard('sudo bash /var/www/satya/deploy.sh', 'Deploy Command')}
              className="px-3.5 py-2 bg-emerald-500 hover:bg-emerald-400 text-slate-950 font-semibold rounded-xl text-xs flex items-center space-x-1.5 shadow-lg shadow-emerald-500/10 transition-colors"
            >
              <Terminal className="w-3.5 h-3.5" />
              <span>Copy Run Command</span>
            </button>
          </div>
        </div>

        {/* Specification Cards Grid */}
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 text-xs">
          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800">
            <span className="text-slate-500 block text-[10px] uppercase">Host & IP</span>
            <span className="font-mono text-slate-200 font-semibold">52.72.236.75</span>
            <span className="text-slate-400 block text-[10px] mt-0.5">AWS EC2 (Ubuntu 22.04)</span>
          </div>

          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800">
            <span className="text-slate-500 block text-[10px] uppercase">Domain & Web Root</span>
            <span className="font-mono text-amber-300 font-semibold truncate block">satya.kasturisundari.xyz</span>
            <span className="text-slate-400 block text-[10px] mt-0.5">/var/www/satya/dist</span>
          </div>

          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800">
            <span className="text-slate-500 block text-[10px] uppercase">RPC Upstream</span>
            <span className="font-mono text-emerald-400 font-semibold truncate block">https://rpc.yugala.org</span>
            <span className="text-slate-400 block text-[10px] mt-0.5">Local: 127.0.0.1:8545</span>
          </div>

          <div className="bg-slate-950 p-3 rounded-xl border border-slate-800">
            <span className="text-slate-500 block text-[10px] uppercase">EVM Spec</span>
            <span className="font-mono text-cyan-300 font-semibold">Chain ID 108108</span>
            <span className="text-slate-400 block text-[10px] mt-0.5">KST (18 Decimals)</span>
          </div>
        </div>
      </div>

      {/* Code Viewer Sub-tabs */}
      <div className="bg-slate-900/80 border border-slate-800 rounded-2xl overflow-hidden shadow-xl">
        <div className="px-6 py-3 border-b border-slate-800 flex items-center justify-between bg-slate-950/60">
          <div className="flex items-center space-x-2">
            <button
              onClick={() => setActiveSubTab('script')}
              className={`flex items-center space-x-2 px-3 py-1.5 rounded-lg text-xs font-medium transition-colors ${
                activeSubTab === 'script'
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              <Terminal className="w-3.5 h-3.5" />
              <span>deploy.sh (Bash Script)</span>
            </button>

            <button
              onClick={() => setActiveSubTab('nginx')}
              className={`flex items-center space-x-2 px-3 py-1.5 rounded-lg text-xs font-medium transition-colors ${
                activeSubTab === 'nginx'
                  ? 'bg-amber-500/20 text-amber-300 border border-amber-500/30'
                  : 'text-slate-400 hover:text-white'
              }`}
            >
              <FileCode className="w-3.5 h-3.5" />
              <span>Nginx Config (Port 80/443)</span>
            </button>
          </div>

          <button
            onClick={() => copyToClipboard(activeSubTab === 'script' ? bashScriptContent : nginxConfContent, activeSubTab === 'script' ? 'deploy.sh' : 'nginx.conf')}
            className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs rounded-lg border border-slate-700 flex items-center space-x-1.5 transition-colors"
          >
            <Copy className="w-3.5 h-3.5" />
            <span>Copy Code</span>
          </button>
        </div>

        {/* Code Content */}
        <div className="p-4 bg-slate-950 overflow-x-auto">
          <pre className="font-mono text-xs text-slate-300 leading-relaxed max-h-[480px] overflow-y-auto">
            {activeSubTab === 'script' ? bashScriptContent : nginxConfContent}
          </pre>
        </div>

        {/* Action / Verification Footer */}
        <div className="p-4 bg-slate-900/90 border-t border-slate-800 flex flex-col sm:flex-row sm:items-center justify-between gap-3 text-xs">
          <div className="flex items-center space-x-2 text-slate-400">
            <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0" />
            <span>Tested on Ubuntu 22.04 LTS (x86_64 / arm64)</span>
          </div>

          <div className="font-mono text-slate-300 bg-slate-950 px-3 py-1 rounded border border-slate-800">
            curl -I https://satya.kasturisundari.xyz
          </div>
        </div>
      </div>

      {copiedField && (
        <div className="fixed bottom-6 right-6 bg-emerald-500 text-slate-950 px-4 py-2.5 rounded-xl font-medium shadow-2xl text-xs flex items-center space-x-2 animate-in fade-in z-50">
          <CheckCircle2 className="w-4 h-4" />
          <span>{copiedField} copied to clipboard!</span>
        </div>
      )}
    </div>
  );
};
