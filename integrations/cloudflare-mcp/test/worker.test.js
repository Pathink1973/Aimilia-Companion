import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
const source = (
  await readFile(new URL("../src/worker.js", import.meta.url), "utf8")
)
  .replace(
    /import\s*\{\s*DurableObject\s*\}\s*from\s*["']cloudflare:workers["'];?/,
    "const DurableObject = class {constructor(ctx,env){this.ctx=ctx;this.env=env;}};",
  )
  .replace(
    /from\s*["']\.\/protocol\.js["']/,
    `from '${new URL("../src/protocol.js", import.meta.url).href}'`,
  )
  .replace(
    /from\s*["']\.\/personal\.js["']/,
    `from '${new URL("../src/personal.js", import.meta.url).href}'`,
  );
const runtimeSource = source
  .replace(
    /from\s*["']\.\/gmail-pages\.js["']/,
    `from '${new URL("../src/gmail-pages.js", import.meta.url).href}'`,
  )
  .replace(
    /from\s*["']\.\/gmail\.js["']/,
    `from '${new URL("../src/gmail.js", import.meta.url).href}'`,
  );
const { default: worker, AimiliaBridge } = await import(
  "data:text/javascript;base64," + Buffer.from(runtimeSource).toString("base64")
);
function context() {
  const map = new Map();
  return {
    storage: {
      async get(k) {
        return map.get(k);
      },
      async put(k, v) {
        map.set(k, v);
      },
      async setAlarm(t) {
        map.set("alarm", t);
      },
      async deleteAlarm() {
        map.delete("alarm");
      },
    },
    blockConcurrencyWhile(fn) {
      this.ready = fn();
      return this.ready;
    },
    waitUntil: (p) => p,
    map,
  };
}
const admin = "not-a-real-secret-01234567890123456789012";
test("Public health reveals no secrets; all control routes need authentication", async () => {
  const env = {
    ADMIN_TOKEN: admin,
    MCP_ENDPOINT: "never-print-this",
    BRIDGE: {
      get() {
        throw Error("must not touch binding");
      },
    },
  };
  const r = await worker.fetch(new Request("https://trial.test/health"), env);
  assert.equal(r.status, 200);
  assert.ok(!(await r.text()).includes("never-print"));
  for (const p of [
    "/api/start",
    "/api/stop",
    "/api/status",
    "/api/probe",
    "/api/enable",
    "/api/storage-check",
    "/api/gmail-check",
  ])
    assert.equal(
      (
        await worker.fetch(
          new Request("https://trial.test" + p, {
            method: p === "/api/status" ? "GET" : "POST",
          }),
          env,
        )
      ).status,
      401,
    );
  const headers = { Authorization: "Bearer " + admin };
  assert.equal(
    (
      await worker.fetch(
        new Request("https://trial.test/unknown", { headers }),
        env,
      )
    ).status,
    404,
  );
});
test("Missing secrets cannot connect or incur a running session", async () => {
  const ctx = context(),
    bridge = new AimiliaBridge(ctx, {});
  await ctx.ready;
  const r = await bridge.fetch(
    new Request("https://bridge.internal/api/start", { method: "POST" }),
  );
  assert.equal(r.status, 503);
  assert.equal(ctx.map.has("alarm"), false);
});
test("Connection, tool discovery and 15-minute expiry use the production bridge", async () => {
  const ctx = context(),
    oldFetch = globalThis.fetch;
  let now = Date.now(),
    oldNow = Date.now;
  Date.now = () => now;
  class Socket extends EventTarget {
    readyState = 1;
    sent = [];
    accept() {}
    send(s) {
      this.sent.push(JSON.parse(s));
    }
    close() {
      this.readyState = 3;
      this.dispatchEvent(new Event("close"));
    }
  }
  const socket = new Socket();
  let connections = 0;
  let handshakeSignal;
  globalThis.fetch = async (url, init) => {
    handshakeSignal = init.signal;
    assert.ok(String(url).startsWith("https://api.xiaozhi.me/mcp/?token="));
    connections++;
    return { status: 101, webSocket: socket };
  };
  try {
    const bridge = new AimiliaBridge(ctx, {
      MCP_ENDPOINT: "wss://api.xiaozhi.me/mcp/?token=FAKE_TEST_ONLY",
    });
    await ctx.ready;
    const r = await bridge.fetch(
      new Request("https://bridge.internal/api/start", { method: "POST" }),
    );
    const data = await r.json();
    assert.equal(data.connected, true);
    assert.equal(handshakeSignal.aborted, false);
    assert.equal(connections, 1);
    assert.equal(ctx.map.get("leaseEnd"), now + 900000);
    await bridge.alarm();
    assert.equal(connections, 1);
    socket.dispatchEvent(
      Object.assign(new Event("message"), {
        data: '{"jsonrpc":"2.0","id":1,"method":"tools/list"}',
      }),
    );
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(socket.sent[0].result.tools.length, 4);
    assert.equal(bridge.discovered, 1);
    assert.equal(bridge.received, 1);
    now += 900001;
    await bridge.alarm();
    assert.equal(socket.readyState, 3);
    assert.equal(ctx.map.get("leaseEnd"), 0);
    assert.equal(ctx.map.has("alarm"), false);
    assert.equal(
      (
        await (
          await bridge.fetch(new Request("https://bridge.internal/api/status"))
        ).json()
      ).status,
      "stopped",
    );
  } finally {
    Date.now = oldNow;
    globalThis.fetch = oldFetch;
  }
});
test("Connection errors cannot leak token or endpoint", async () => {
  const ctx = context(),
    old = globalThis.fetch;
  globalThis.fetch = async () => {
    throw Error("token=PRIVATE");
  };
  try {
    const bridge = new AimiliaBridge(ctx, {
      MCP_ENDPOINT: "wss://api.xiaozhi.me/mcp/?token=PRIVATE",
    });
    await ctx.ready;
    const data = await (
      await bridge.fetch(
        new Request("https://bridge.internal/api/start", { method: "POST" }),
      )
    ).json();
    assert.equal(data.status, "connection_failed");
    assert.ok(!JSON.stringify(data).includes("PRIVATE"));
    assert.ok(ctx.map.get("alarm") > Date.now());
    await bridge.stop();
  } finally {
    globalThis.fetch = old;
  }
});

test("Permanent mode survives reconstruction and cron resume cannot restart a stopped service", async () => {
  const ctx = context(),
    priorFetch = globalThis.fetch;
  let sockets = 0;
  globalThis.fetch = async () => ({
    status: 101,
    webSocket: {
      readyState: 1,
      accept() {},
      addEventListener() {},
      close() {
        this.readyState = 3;
        sockets++;
      },
    },
  });
  try {
    const env = { MCP_ENDPOINT: "wss://api.xiaozhi.me/mcp/?token=TEST_ONLY" };
    let bridge = new AimiliaBridge(ctx, env);
    await ctx.ready;
    let data = await (
      await bridge.fetch(
        new Request("https://bridge.internal/api/enable", { method: "POST" }),
      )
    ).json();
    assert.equal(data.mode, "permanent");
    assert.equal(data.lease_expires_at, null);
    // Simulate object eviction; persistent state is shared, in-memory socket is not.
    bridge = new AimiliaBridge(ctx, env);
    await ctx.ready;
    data = await (
      await bridge.fetch(
        new Request("https://bridge.internal/api/resume", { method: "POST" }),
      )
    ).json();
    assert.equal(data.connected, true);
    assert.equal(data.mode, "permanent");
    await bridge.stop();
    await bridge.fetch(
      new Request("https://bridge.internal/api/resume", { method: "POST" }),
    );
    assert.equal(bridge.ws, null);
    assert.equal(ctx.map.get("permanent"), false);
    assert.equal(ctx.map.has("alarm"), false);
  } finally {
    globalThis.fetch = priorFetch;
  }
});

test("Handshake timeout is cancelled after upgrade and cannot abort the established socket", async () => {
  const priorFetch = globalThis.fetch,
    priorSet = globalThis.setTimeout,
    priorClear = globalThis.clearTimeout;
  let timer,
    cleared = false,
    signal;
  globalThis.setTimeout = (fn, ms) => {
    assert.equal(ms, 10000);
    timer = { fn };
    return timer;
  };
  globalThis.clearTimeout = (id) => {
    if (id === timer) cleared = true;
  };
  globalThis.fetch = async (_url, init) => {
    signal = init.signal;
    return {
      status: 101,
      webSocket: { readyState: 1, accept() {}, addEventListener() {} },
    };
  };
  try {
    const ctx = context(),
      bridge = new AimiliaBridge(ctx, {
        MCP_ENDPOINT: "wss://api.xiaozhi.me/mcp/?token=TEST_ONLY",
      });
    await ctx.ready;
    bridge.leaseEnd = Date.now() + 60000;
    await bridge.ensureConnected();
    assert.equal(cleared, true);
    assert.equal(signal.aborted, false);
  } finally {
    globalThis.fetch = priorFetch;
    globalThis.setTimeout = priorSet;
    globalThis.clearTimeout = priorClear;
  }
});
