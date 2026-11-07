// Local authorization only. No Google credentials or email content are logged.
import { createServer } from "node:http";
import { readFile, writeFile, mkdir } from "node:fs/promises";
import { randomBytes, createHash, timingSafeEqual } from "node:crypto";
import { GMAIL_SCOPE } from "../src/gmail.js";
const secretDir = new URL("../.secrets/", import.meta.url);
const config = JSON.parse(
  await readFile(new URL("aimilia-gmail-client.json", secretDir), "utf8"),
);
const c = config.web;
const redirect = "http://localhost:8768/oauth/callback";
if (
  !c?.client_id?.endsWith(".apps.googleusercontent.com") ||
  !c.client_secret ||
  !c.redirect_uris?.includes(redirect)
)
  throw Error(
    "Cliente OAuth não preparado. Configura o callback localhost:8768 e guarda o JSON no ficheiro privado.",
  );
const state = randomBytes(32).toString("hex"),
  verifier = randomBytes(32).toString("base64url");
const challenge = createHash("sha256").update(verifier).digest("base64url");
let used = false, completed = false;
const equal = (a, b) =>
  typeof a === "string" &&
  a.length === b.length &&
  timingSafeEqual(Buffer.from(a), Buffer.from(b));
function page(res, status, message) {
  res.writeHead(status, {
    "Content-Type": "text/html; charset=utf-8",
    "Cache-Control": "no-store",
    "Referrer-Policy": "no-referrer",
    "Content-Security-Policy":
      "default-src 'none'; style-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'",
  });
  res.end(
    '<!doctype html><meta charset="utf-8"><title>Aimilia Gmail</title><main style="font:18px system-ui;max-width:650px;margin:60px auto"><h1>Aimilia · Gmail</h1>' +
      message +
      "</main>",
  );
}
const server = createServer(async (req, res) => {
  try {
    if (req.headers.host !== "localhost:8768" || req.method !== "GET") {
      page(res, 403, "Pedido recusado.");
      return;
    }
    const u = new URL(req.url, "http://localhost:8768");
    if (u.pathname === "/complete" && completed) {
      page(res,200,"<p>Autorização de leitura recebida. A credencial foi guardada no ficheiro privado local para instalação em Cloudflare Secrets. Podes fechar esta página.</p>");
      return;
    }
    if (u.pathname === "/") {
      page(
        res,
        200,
        '<p>Autoriza apenas a leitura do teu Gmail. Emails consultados pela Aimilia passam pelo Cloudflare e pelo serviço XiaoZhi para gerar a resposta por voz. Não enviamos nem apagamos mensagens.</p><p><a href="/authorize">Continuar para o Google</a></p>',
      );
      return;
    }
    if (u.pathname === "/authorize" && !used) {
      const auth = new URL("https://accounts.google.com/o/oauth2/v2/auth");
      for (const [k, v] of Object.entries({
        client_id: c.client_id,
        redirect_uri: redirect,
        response_type: "code",
        scope: GMAIL_SCOPE,
        access_type: "offline",
        prompt: "consent select_account",
        state,
        code_challenge: challenge,
        code_challenge_method: "S256",
      }))
        auth.searchParams.set(k, v);
      res.writeHead(302, {
        Location: auth.href,
        "Cache-Control": "no-store",
        "Referrer-Policy": "no-referrer",
      });
      res.end();
      return;
    }
    if (
      u.pathname !== "/oauth/callback" ||
      used ||
      !equal(u.searchParams.get("state"), state)
    ) {
      page(
        res,
        400,
        "Sessão inválida ou já utilizada. Reinicia a autorização.",
      );
      return;
    }
    used = true;
    if (u.searchParams.has("error")) {
      page(
        res,
        400,
        "Autorização não concluída. Não foi guardado nenhum acesso.",
      );
      return;
    }
    const code = u.searchParams.get("code");
    if (!code || code.length > 4096) throw Error("invalid");
    const response = await fetch("https://oauth2.googleapis.com/token", {
      method: "POST",
      redirect: "error",
      signal: AbortSignal.timeout(15000),
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: new URLSearchParams({
        client_id: c.client_id,
        client_secret: c.client_secret,
        redirect_uri: redirect,
        grant_type: "authorization_code",
        code,
        code_verifier: verifier,
      }),
    });
    if (!response.ok) throw Error("exchange_failed");
    const t = await response.json();
    if (!t.refresh_token || t.scope !== GMAIL_SCOPE)
      throw Error("scope_or_refresh_invalid");
    await mkdir(secretDir, { recursive: true, mode: 0o700 });
    await writeFile(
      new URL("aimilia-gmail-secrets.json", secretDir),
      JSON.stringify({
        GMAIL_CLIENT_ID: c.client_id,
        GMAIL_CLIENT_SECRET: c.client_secret,
        GMAIL_REFRESH_TOKEN: t.refresh_token,
      }),
      { mode: 0o600, flag: "wx" },
    );
    await writeFile(new URL("aimilia-gmail-authorization.json",secretDir),JSON.stringify({scope:t.scope,refresh_token_expires_in:Number.isFinite(t.refresh_token_expires_in)?t.refresh_token_expires_in:null}),{mode:0o600});
    completed=true;
    res.writeHead(303,{Location:"/complete","Cache-Control":"no-store","Referrer-Policy":"no-referrer"});
    res.end();
    console.log(
      "Autorização Gmail recebida; ficheiro privado pronto. Nenhum email foi lido.",
    );
  } catch {
    page(
      res,
      500,
      "Não foi possível concluir a autorização. Não copies códigos ou palavras-passe para o chat.",
    );
    console.error("Autorização não concluída. Detalhes sensíveis omitidos.");
  }
});
server.listen(8768, "127.0.0.1", () =>
  console.log(
    "Autorização local pronta: http://localhost:8768/ (expira em 15 minutos)",
  ),
);
setTimeout(() => server.close(), 15 * 60 * 1000).unref();
