const http = require('node:http');
const { timingSafeEqual } = require('node:crypto');

const PORT = Number(process.env.PORT || 9123);
const HOST = process.env.HOST || '127.0.0.1';
const API_KEY = process.env.CALC_API_KEY || '';
const DISCORD_ENABLED = false;
const USER_TOKEN = DISCORD_ENABLED ? process.env.DISCORD_USER_TOKEN : '';
const GROQ_API_KEY = process.env.GROQ_API_KEY || '';
const GROQ_MODEL = process.env.GROQ_MODEL || 'openai/gpt-oss-20b';

if (API_KEY.length < 32) throw new Error('Set CALC_API_KEY to a random secret of at least 32 characters.');
if (DISCORD_ENABLED && !USER_TOKEN) throw new Error('Set DISCORD_USER_TOKEN to your user account token.');

const client = DISCORD_ENABLED
  ? new (require('discord.js-selfbot-v13').Client)({
      checkUpdate: false,
      ws: {
        properties: {
          $browser: 'Discord iOS',
        },
      },
    })
  : null;

const dmChannels = new Map();
const SUPPORTED_TEXT_CHARACTERS = new Set([
  ' ',
  ...'qwertyuiop', ..."asdfghjkl'", ...'zxcvbnm,.?',
  ...'QWERTYUIOP', ...'ASDFGHJKL"', ...'ZXCVBNM;:!',
  ...'1234567890', ...'+-*/=%().,', ...'<>[]{}:;!?',
  ...'!@#$%^&*()', ...'-_=+[]{}|\\', ...';:\'",.<>/?', ...'`~',
]);
const AI_HISTORY_LIMIT = 2;
const aiHistory = [];
const AI_SYSTEM_PROMPT = `Answer directly with useful, specific information. Keep replies concise and avoid filler or repetition. Use only these characters in your answer: ${[...SUPPORTED_TEXT_CHARACTERS].join('')}. Spaces are allowed. Use normal spaces between words; do not omit them or replace them with underscores.`;

// Inactivity Timer Configuration (10 minutes = 600,000 ms)
const INACTIVITY_TIMEOUT_MS = 10 * 60 * 1000;
let inactivityTimer = null;
let isConnecting = false;

function resetInactivityTimer() {
  if (!DISCORD_ENABLED) return;
  if (inactivityTimer) clearTimeout(inactivityTimer);
  inactivityTimer = setTimeout(async () => {
    if (client.isReady()) {
      console.log('10 minutes of inactivity detected. Disconnecting self-bot account...');
      await client.destroy();
      console.log('Self-bot disconnected and offline.');
    }
  }, INACTIVITY_TIMEOUT_MS);
}

// Ensures Discord client is connected before processing requests
async function ensureConnected() {
  resetInactivityTimer(); // Reset timer on any incoming request
  
  if (client.isReady()) return;
  if (isConnecting) {
    // Wait if connection is already in progress
    while (isConnecting && !client.isReady()) {
      await new Promise((resolve) => setTimeout(resolve, 200));
    }
    return;
  }

  try {
    isConnecting = true;
    console.log('New request received while offline. Reconnecting to Discord...');
    await client.login(USER_TOKEN);
    console.log(`Reconnected successfully as ${client.user.tag}`);
  } catch (error) {
    console.error(`Reconnection failed: ${error.message}`);
    throw error;
  } finally {
    isConnecting = false;
  }
}

function sanitizeText(value) {
  let sanitized = '';
  for (const character of value) {
    if (SUPPORTED_TEXT_CHARACTERS.has(character)) sanitized += character;
  }
  return sanitized;
}

function sanitizeJsonValue(value) {
  if (typeof value === 'string') return sanitizeText(value);
  if (Array.isArray(value)) return value.map(sanitizeJsonValue);
  if (value && typeof value === 'object') {
    return Object.fromEntries(Object.entries(value).map(([key, item]) => [key, sanitizeJsonValue(item)]));
  }
  return value;
}

function sendJson(res, status, data) {
  res.writeHead(status, {
    'content-type': 'application/json; charset=utf-8',
    'cache-control': 'no-store',
    'x-content-type-options': 'nosniff',
  });
  res.end(JSON.stringify(sanitizeJsonValue(data)));
}

function authorized(req) {
  const supplied = Buffer.from(String(req.headers['x-calc-key'] || ''));
  const expected = Buffer.from(API_KEY);
  return supplied.length === expected.length && timingSafeEqual(supplied, expected);
}

async function readJson(req) {
  let body = '';
  for await (const chunk of req) {
    body += chunk;
    if (Buffer.byteLength(body) > 8192) throw new Error('Request body too large.');
  }
  return body ? JSON.parse(body) : {};
}

async function loadDmChannels() {
  const items = [];
  try {
    const rawChannels = await client.api.users('@me', 'channels').get();
    for (const channelData of rawChannels) {
      if (channelData.type === 1) {
        let channel = client.channels.cache.get(channelData.id);
        if (!channel) {
          channel = client.channels.cache.set(channelData.id, client.channels.add(channelData));
        }
        dmChannels.set(channelData.id, channel);

        const recipient = channelData.recipients?.[0];
        items.push({
          id: channelData.id,
          name: recipient ? (recipient.global_name || recipient.username || recipient.id) : channelData.id,
          username: recipient?.username || '',
          last: channelData.last_message_id || '',
        });
      }
    }
  } catch (error) {
    console.error(`Error loading DMs: ${error.message}`);
  }
  return items;
}

async function handle(req, res) {
  if (!authorized(req)) return sendJson(res, 401, { error: 'unauthorized' });

  const url = new URL(req.url, 'http://localhost');

  if (
    !DISCORD_ENABLED &&
    (url.pathname === '/api/dms' || /^\/api\/dms\/\d+\/messages$/.test(url.pathname))
  ) {
    return sendJson(res, 503, { error: 'discord_disabled' });
  }

  // GET /api/dms — Returns ALL active DMs automatically
  if (req.method === 'GET' && url.pathname === '/api/dms') {
    await ensureConnected();
    return sendJson(res, 200, { items: await loadDmChannels() });
  }

  const messageMatch = url.pathname.match(/^\/api\/dms\/(\d+)\/messages$/);
  if (messageMatch) {
    await ensureConnected();
    const channelId = messageMatch[1];
    let channel = dmChannels.get(channelId);

    if (!channel) {
      try {
        channel = await client.channels.fetch(channelId);
        if (channel && channel.type === 'DM') {
          dmChannels.set(channelId, channel);
        }
      } catch (_) {
        channel = null;
      }
    }

    if (!channel || channel.type !== 'DM') {
      return sendJson(res, 404, { error: 'dm_not_found' });
    }

    // GET /api/dms/:channelId/messages
    if (req.method === 'GET') {
      const requested = Number(url.searchParams.get('limit') || 12);
      const limit = Math.max(1, Math.min(20, Number.isFinite(requested) ? requested : 12));
      const messages = await channel.messages.fetch({ limit });
      const items = [...messages.values()].reverse().map((message) => ({
        id: message.id,
        author: message.author.id === client.user.id ? 'bot' : (message.author.username || 'user'),
        content: message.content || '',
        attachments: message.attachments.size,
      }));
      return sendJson(res, 200, { items });
    }

    // POST /api/dms/:channelId/messages
    if (req.method === 'POST') {
      const body = await readJson(req);
      const content = typeof body.content === 'string' ? sanitizeText(body.content).trim() : '';
      if (!content || content.length > 2000) return sendJson(res, 400, { error: 'invalid_message' });
      const message = await channel.send({ content });
      return sendJson(res, 201, { id: message.id });
    }
  }

  // POST /api/ai
  if (req.method === 'POST' && url.pathname === '/api/ai') {
    resetInactivityTimer();
    if (!GROQ_API_KEY) return sendJson(res, 503, { error: 'groq_not_configured' });
    const body = await readJson(req);
    const prompt = typeof body.prompt === 'string' ? sanitizeText(body.prompt).trim() : '';
    if (!prompt || prompt.length > 2000) return sendJson(res, 400, { error: 'invalid_prompt' });

    const response = await fetch('https://api.groq.com/openai/v1/chat/completions', {
      method: 'POST',
      headers: {
        'content-type': 'application/json',
        authorization: `Bearer ${GROQ_API_KEY}`,
      },
      body: JSON.stringify({
        model: GROQ_MODEL,
        messages: [
          { role: 'system', content: AI_SYSTEM_PROMPT },
          ...aiHistory,
          { role: 'user', content: prompt },
        ],
      }),
      signal: AbortSignal.timeout(25000),
    });
    const result = await response.json();
    if (!response.ok) {
      console.warn(`Groq returned HTTP ${response.status}`);
      return sendJson(res, 502, { error: 'groq_request_failed' });
    }
    const text = result.choices?.[0]?.message?.content;
    const answer = typeof text === 'string' ? sanitizeText(text).slice(0, 5000).trim() : '';
    if (!answer) {
      return sendJson(res, 502, { error: 'empty_ai_response' });
    }
    aiHistory.push(
      { role: 'user', content: prompt },
      { role: 'assistant', content: answer },
    );
    if (aiHistory.length > AI_HISTORY_LIMIT) {
      aiHistory.splice(0, aiHistory.length - AI_HISTORY_LIMIT);
    }
    return sendJson(res, 200, { text: answer });
  }

  return sendJson(res, 404, { error: 'not_found' });
}

const server = http.createServer((req, res) => {
  const started = process.hrtime.bigint();
  const pathname = String(req.url || '/').split('?')[0];
  const route = pathname.replace(/^\/api\/dms\/\d+\/messages$/, '/api/dms/:channelId/messages');
  let logged = false;
  const logRequest = () => {
    if (logged) return;
    logged = true;
    const durationMs = Number(process.hrtime.bigint() - started) / 1e6;
    console.log(`${req.method} ${route} ${res.statusCode} ${durationMs.toFixed(1)}ms`);
  };
  res.once('finish', logRequest);
  res.once('close', logRequest);

  handle(req, res).catch((error) => {
    console.error(error.message);
    if (!res.headersSent) sendJson(res, 500, { error: 'request_failed' });
    else res.destroy();
  });
});

if (DISCORD_ENABLED) {
  client.on('ready', async () => {
    console.log(`Self-bot online as ${client.user.tag} (Idle / Mobile)`);
    
    client.user.setPresence({
      status: 'idle',
      activities: [],
    });

    const dms = await loadDmChannels();
    console.log(`Auto-loaded ${dms.length} active DM conversations:`);
    dms.forEach((dm) => console.log(` - ${dm.name} (${dm.id})`));
    
    resetInactivityTimer();
  });

  client.on('messageCreate', (message) => {
    if (message.channel.type === 'DM') {
      dmChannels.set(message.channel.id, message.channel);
      resetInactivityTimer();
    }
  });

  client.on('error', (error) => console.error(`Discord error: ${error.message}`));
}

server.listen(PORT, HOST, () => console.log(`API listening on http://${HOST}:${PORT}`));

if (DISCORD_ENABLED) {
  client.login(USER_TOKEN).catch((error) => {
    console.error(`Login failed: ${error.message}`);
    process.exitCode = 1;
    server.close();
  });
} else {
  console.log('Discord integration disabled.');
}

for (const signal of ['SIGINT', 'SIGTERM']) {
  process.on(signal, () => {
    if (inactivityTimer) clearTimeout(inactivityTimer);
    server.close();
    if (DISCORD_ENABLED) client.destroy();
  });
}