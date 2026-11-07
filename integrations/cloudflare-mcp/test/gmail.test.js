import test from "node:test";
import assert from "node:assert/strict";
import { gmailService, messageSummary, GMAIL_SCOPE } from "../src/gmail.js";
import { createHandler } from "../src/protocol.js";
const env = {
  GMAIL_CLIENT_ID: "FAKE",
  GMAIL_CLIENT_SECRET: "NEVER_PRINT",
  GMAIL_REFRESH_TOKEN: "TEST_ONLY",
};
const json = (x) => Response.json(x);
test("Gmail is absent from discovery until all separate credentials exist", async () => {
  const disabled = gmailService({});
  const h = createHandler({ gmail: disabled });
  assert.equal(
    (await h('{"jsonrpc":"2.0","id":1,"method":"tools/list"}')).result.tools
      .length,
    4,
  );
  await assert.rejects(disabled.call({ action: "resumo" }));
});
test("Only token POST and fixed Gmail GET requests are possible; tokens cached, reads do not modify labels", async () => {
  const requests = [];
  let tokens = 0;
  const service = gmailService(env, {
    fetcher: async (url, init) => {
      requests.push({ url: String(url), method: init.method });
      if (String(url).includes("oauth2.googleapis.com")) {
        tokens++;
        assert.ok(init.body instanceof URLSearchParams);
        return json({
          access_token: "temporary",
          token_type: "Bearer",
          expires_in: 3600,
          scope: GMAIL_SCOPE,
        });
      }
      assert.equal(init.method, "GET");
      assert.ok(
        String(url).startsWith(
          "https://gmail.googleapis.com/gmail/v1/users/me/",
        ),
      );
      if (String(url).includes("labels/INBOX"))
        return json({ messagesUnread: 2, messagesTotal: 5 });
      if (new URL(url).pathname.endsWith("/messages"))
        return json({ messages: [{ id: "abcdef1234" }] });
      return json({
        id: "abcdef1234",
        snippet: "Resumo",
        payload: {
          headers: [{ name: "Subject", value: "Teste" }],
          mimeType: "text/plain",
          body: { data: btoa("Texto") },
        },
      });
    },
  });
  assert.equal((await service.call({ action: "resumo" })).nao_lidos, 2);
  assert.equal(
    (await service.call({ action: "pesquisar" })).mensagens[0].assunto,
    "Teste",
  );
  assert.equal(
    (await service.call({ action: "ler", id: "abcdef1234" })).mensagem.texto,
    "Texto",
  );
  assert.equal(tokens, 1);
  assert.ok(
    requests.every(
      (r) =>
        r.method === "GET" || r.url === "https://oauth2.googleapis.com/token",
    ),
  );
  await assert.rejects(service.call({ action: "apagar", id: "abcdef1234" }));
  await assert.rejects(service.call({ action: "ler", id: "../../other" }));
  await assert.rejects(service.call({ action: "pesquisar", limite: 6 }));
});
test("MIME body ignores attachments, remote scripts and returns bounded external content", () => {
  const payload = {
    mimeType: "multipart/mixed",
    parts: [
      { mimeType: "text/plain", body: { data: Buffer.from("Olá email", "utf8").toString("base64url") } },
      {
        filename: "segredo.txt",
        mimeType: "text/plain",
        body: { data: btoa("NAO_LER") },
      },
    ],
  };
  const r = messageSummary({ id: "abc", payload }, true);
  assert.equal(r.texto, "Olá email");
  assert.equal(r.anexos_incluidos, false);
  const html = messageSummary(
    {
      payload: {
        mimeType: "text/html",
        body: {
          data: btoa(
            '<script>evil()</script><img src="https://tracker.test">texto',
          ),
        },
      },
    },
    true,
  );
  assert.equal(html.texto, "texto");
});
test("No credential, provider error or broad scope leaks through tool errors", async () => {
  for (const mode of ["failure", "broad"]) {
    const service = gmailService(env, {
      fetcher: async () =>
        mode === "failure"
          ? new Response("NEVER_PRINT", { status: 400 })
          : json({
              access_token: "temporary",
              token_type: "Bearer",
              scope: "https://mail.google.com/",
            }),
    });
    const h = createHandler({ gmail: service });
    const r = await h(
      JSON.stringify({
        jsonrpc: "2.0",
        id: 1,
        method: "tools/call",
        params: { name: "aimilia_gmail", arguments: { action: "resumo" } },
      }),
    );
    assert.equal(r.result.isError, true);
    assert.ok(!JSON.stringify(r).includes("NEVER_PRINT"));
  }
});

test("OAuth denial exposes only allowlisted diagnostic fields, never provider descriptions", async () => {
  const service = gmailService(env, { fetcher: async () => Response.json({
    error: "invalid_grant", error_description: "PRIVATE_TOKEN_SHOULD_NOT_ESCAPE"
  }, {status:400}) });
  await assert.rejects(service.call({action:"resumo"}), e => {
    assert.equal(e.provider_stage,"oauth"); assert.equal(e.provider_status,400);
    assert.equal(e.provider_code,"invalid_grant");
    assert.ok(!JSON.stringify(e).includes("PRIVATE_TOKEN")); return true;
  });
});
