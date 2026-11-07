import { gmailPage } from "./gmail-pages.js";
import { gmailService } from "./gmail.js";
import { DurableObject } from "cloudflare:workers";
import { createHandler, validateEndpoint } from "./protocol.js";
import { personalService } from "./personal.js";
const json = (data, status = 200) =>
  Response.json(data, {
    status,
    headers: {
      "Cache-Control": "no-store",
      "X-Content-Type-Options": "nosniff",
    },
  });
function authorized(request, env) {
  const expected = env.ADMIN_TOKEN;
  if (!expected || expected.length < 32) return false;
  const actual = request.headers.get("Authorization") ?? "",
    valid = `Bearer ${expected}`;
  if (actual.length !== valid.length) return false;
  let difference = 0;
  for (let i = 0; i < valid.length; i++)
    difference |= actual.charCodeAt(i) ^ valid.charCodeAt(i);
  return difference === 0;
}
export default {
  async scheduled(_event, env, ctx) {
    const stub = env.BRIDGE.get(env.BRIDGE.idFromName("aimilia-service-v1"));
    ctx.waitUntil(
      stub.fetch(
        new Request("https://bridge.internal/api/resume", { method: "POST" }),
      ),
    );
  },
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "GET") {
      const page = gmailPage(url.pathname);
      if (page)
        return new Response(page, {
          headers: {
            "Content-Type": "text/html; charset=utf-8",
            "Cache-Control": "public, max-age=300",
            "Content-Security-Policy":
              "default-src 'none'; style-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'",
            "X-Content-Type-Options": "nosniff",
            "Referrer-Policy": "no-referrer",
          },
        });
    }
    if (url.pathname === "/health" && request.method === "GET")
      return json({ service: "aimilia-mcp-trial", version: "0.3.0" });
    if (!authorized(request, env)) return json({ error: "unauthorized" }, 401);
    const path = url.pathname;
    if (path === "/api/gmail-check" && request.method === "POST" && url.searchParams.get("fresh") === "1") {
      try {
        await gmailService(env).call({action:"resumo"});
        return json({gmail_readonly_connection_ok:true});
      } catch(error) {
        return json({gmail_readonly_connection_ok:false,
          provider_stage:error?.provider_stage ?? "unknown",
          provider_status:error?.provider_status ?? null,
          provider_code:error?.provider_code ?? "connection_error"},503);
      }
    }
    if (
      !(
        (request.method === "GET" && path === "/api/status") ||
        (request.method === "POST" &&
          [
            "/api/start",
            "/api/stop",
            "/api/probe",
            "/api/enable",
            "/api/storage-check",
            "/api/gmail-check",
          ].includes(path))
      )
    )
      return json({ error: "not_found" }, 404);
    const stub = env.BRIDGE.get(env.BRIDGE.idFromName("aimilia-service-v1"));
    return stub.fetch(
      new Request(`https://bridge.internal${path}`, { method: request.method }),
    );
  },
};
export class AimiliaBridge extends DurableObject {
  constructor(ctx, env) {
    super(ctx, env);
    this.ctx = ctx;
    this.env = env;
    this.ws = null;
    this.connecting = false;
    this.leaseEnd = 0;
    this.permanent = false;
    this.status = "stopped";
    this.attempts = 0;
    this.inflight = 0;
    this.generation = 0;
    this.calls = 0;
    this.received = 0;
    this.discovered = 0;
    this.closeCode = null;
    this.connectionStage = "idle";
    this.upstreamStatus = null;
    this.failureType = null;
    this.gmail = gmailService(env);
    this.handle = createHandler({
      personal: personalService(ctx.storage),
      gmail: this.gmail,
    });
    ctx.blockConcurrencyWhile(async () => {
      this.permanent = (await ctx.storage.get("permanent")) === true;
      this.leaseEnd = this.permanent
        ? 8640000000000000
        : ((await ctx.storage.get("leaseEnd")) ?? 0);
    });
  }
  async fetch(request) {
    const path = new URL(request.url).pathname;
    if (path === "/api/gmail-check") {
      try {
        await this.gmail.call({ action: "resumo" });
        return json({ gmail_readonly_connection_ok: true });
      } catch (error) {
        return json({ gmail_readonly_connection_ok: false,
          provider_stage: error?.provider_stage ?? "unknown",
          provider_status: error?.provider_status ?? null,
          provider_code: error?.provider_code ?? "connection_error",
        }, 503);
      }
    }
    if (path === "/api/storage-check") {
      const key = "diagnostic." + crypto.randomUUID();
      const isolated = {
        transaction: (fn) =>
          this.ctx.storage.transaction((tx) =>
            fn({
              get: () => tx.get(key),
              put: (_k, value) => tx.put(key, value),
            }),
          ),
      };
      try {
        let use = personalService(isolated);
        await use("aimilia_preferencias", {
          action: "guardar",
          chave: "teste",
          valor: "sintetico",
          confirmado: true,
        });
        // Recreate service to ensure it reads persisted data rather than a local cache.
        use = personalService(isolated);
        const prefs = await use("aimilia_preferencias", { action: "listar" });
        const event = await use("aimilia_agenda", {
          action: "criar",
          titulo: "Teste sintético",
          data_hora: "2030-01-01T09:00:00Z",
          confirmado: true,
        });
        const entries = await use("aimilia_agenda", {
          action: "listar",
          filtro: "todos",
        });
        await use("aimilia_agenda", {
          action: "alterar",
          id: event.entrada.id,
          titulo: "Atualizado",
          confirmado: true,
        });
        await use("aimilia_agenda", {
          action: "apagar",
          id: event.entrada.id,
          confirmado: true,
        });
        await use("aimilia_preferencias", {
          action: "apagar",
          chave: "teste",
          confirmado: true,
        });
        const empty = await this.ctx.storage.get(key);
        return json({
          storage_write_read_ok: prefs.preferencias[0]?.valor === "sintetico",
          agenda_crud_ok: entries.entradas.length === 1,
          cleanup_ok:
            empty.entries.length === 0 &&
            Object.keys(empty.preferences).length === 0,
        });
      } finally {
        await this.ctx.storage.delete(key);
      }
    }
    if (path === "/api/resume") {
      if (this.permanent) await this.ensureConnected();
    }
    if (path === "/api/probe") {
      const result = await this.handle(
        JSON.stringify({
          jsonrpc: "2.0",
          id: 1,
          method: "tools/call",
          params: {
            name: "aimilia_noticias_portugal",
            arguments: { limit: 1 },
          },
        }),
      );
      return json({
        news_probe_ok: result?.result?.isError === false,
        origin: "RTP",
        via: "Cloudflare",
      });
    }
    if (path === "/api/start" || path === "/api/enable") {
      if (!this.env.MCP_ENDPOINT)
        return json({ error: "mcp_secret_missing" }, 503);
      try {
        validateEndpoint(this.env.MCP_ENDPOINT);
      } catch {
        return json({ error: "invalid_mcp_endpoint" }, 400);
      }
      this.permanent = path === "/api/enable";
      this.leaseEnd = this.permanent
        ? 8640000000000000
        : Date.now() + 15 * 60000;
      await this.ctx.storage.put("permanent", this.permanent);
      await this.ctx.storage.put("leaseEnd", this.leaseEnd);
      await this.ensureConnected();
    } else if (path === "/api/stop") await this.stop();
    return json({
      status: this.status,
      connected: this.ws?.readyState === 1,
      service_version: "0.3.0",
      gmail_configured: this.gmail.enabled,
      mode: this.permanent ? "permanent" : "trial",
      lease_expires_at:
        !this.permanent && this.leaseEnd
          ? new Date(this.leaseEnd).toISOString()
          : null,
      tool_calls: this.calls,
      messages_received: this.received,
      tool_discoveries: this.discovered,
      last_close_code: this.closeCode,
      connection_stage: this.connectionStage,
      upstream_http_status: this.upstreamStatus,
      failure_type: this.failureType,
    });
  }
  async alarm() {
    if (Date.now() >= this.leaseEnd) {
      await this.stop();
      return;
    }
    await this.ensureConnected();
  }
  async ensureConnected() {
    if (this.connecting) return;
    if (Date.now() >= this.leaseEnd) {
      await this.stop();
      return;
    }
    if (this.ws?.readyState === 1) {
      await this.ctx.storage.setAlarm(
        Math.min(this.leaseEnd, Date.now() + 30000),
      );
      return;
    }
    this.connecting = true;
    this.status = "connecting";
    const generation = ++this.generation;
    try {
      this.connectionStage = "validate";
      const url = validateEndpoint(this.env.MCP_ENDPOINT);
      url.protocol = "https:";
      this.connectionStage = "fetch_upgrade";
      const controller = new AbortController();
      const handshakeTimer = setTimeout(() => controller.abort(), 10000);
      let response;
      try {
        response = await fetch(url.toString(), {
          headers: { Upgrade: "websocket" },
          redirect: "manual",
          signal: controller.signal,
        });
      } finally {
        clearTimeout(handshakeTimer);
      }
      const ws = response.webSocket;
      this.upstreamStatus = response.status;
      if (response.status !== 101 || !ws) throw Error("upgrade_failed");
      this.connectionStage = "accept_socket";
      ws.accept();
      if (generation !== this.generation || Date.now() >= this.leaseEnd) {
        ws.close(1000, "Service stopped");
        return;
      }
      this.ws = ws;
      this.status = "connected";
      this.attempts = 0;
      this.connectionStage = "connected";
      this.failureType = null;
      ws.addEventListener("message", (event) => {
        if (generation !== this.generation || Date.now() >= this.leaseEnd)
          return;
        if (
          typeof event.data !== "string" ||
          event.data.length > 32768 ||
          this.inflight >= 4
        ) {
          ws.close(1008, "Message limit");
          return;
        }
        this.inflight++;
        this.ctx.waitUntil(
          (async () => {
            try {
              this.received++;
              const method = JSON.parse(event.data)?.method;
              if (method === "tools/list") this.discovered++;
              const reply = await this.handle(event.data);
              if (reply && ws.readyState === 1) ws.send(JSON.stringify(reply));
              if (reply?.result?.content) this.calls++;
            } catch {
              if (ws.readyState === 1) ws.close(1008, "Invalid message");
            } finally {
              this.inflight--;
            }
          })(),
        );
      });
      const closed = (event) => {
        if (typeof event.code === "number") this.closeCode = event.code;
        if (generation !== this.generation) return;
        this.ws = null;
        this.status = "disconnected";
        if (Date.now() < this.leaseEnd)
          this.ctx.waitUntil(
            this.ctx.storage.setAlarm(
              Math.min(this.leaseEnd, Date.now() + 3000),
            ),
          );
      };
      ws.addEventListener("close", closed);
      ws.addEventListener("error", closed);
    } catch (error) {
      this.status = "connection_failed";
      this.failureType = [
        "Error",
        "TypeError",
        "TimeoutError",
        "AbortError",
      ].includes(error?.name)
        ? error.name
        : "OtherError";
      this.attempts++;
    } finally {
      this.connecting = false;
      if (generation === this.generation && Date.now() < this.leaseEnd)
        await this.ctx.storage.setAlarm(
          Math.min(
            this.leaseEnd,
            Date.now() +
              Math.min(60000, 3000 * 2 ** Math.min(this.attempts, 5)),
          ),
        );
    }
  }
  async stop() {
    this.leaseEnd = 0;
    this.generation++;
    const ws = this.ws;
    this.ws = null;
    this.status = "stopped";
    if (ws) {
      try {
        ws.close(1000, "Service stopped");
      } catch {}
    }
    this.permanent = false;
    await this.ctx.storage.put("permanent", false);
    await this.ctx.storage.put("leaseEnd", 0);
    await this.ctx.storage.deleteAlarm();
  }
}
