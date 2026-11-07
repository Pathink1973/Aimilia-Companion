import test from "node:test";
import assert from "node:assert/strict";
import {
  createHandler,
  parseNews,
  validateEndpoint,
  boundedText,
  TOOLS,
} from "../src/protocol.js";
const xml = `<rss><channel><item><title><![CDATA[Notícia &amp; informação]]></title><link>https://www.rtp.pt/noticias/pais/exemplo_n1</link><pubDate>Sun, 25 Oct 2026 10:00:00 GMT</pubDate></item><item><title>Outra notícia</title><link>https://www.rtp.pt/noticias/exemplo_n2</link></item></channel></rss>`;
const call = (name, args = {}) =>
  JSON.stringify({
    jsonrpc: "2.0",
    id: 4,
    method: "tools/call",
    params: { name, arguments: args },
  });
test("MCP initialize, discovery, ping and notification lifecycle", async () => {
  const handle = createHandler();
  const init = await handle('{"jsonrpc":"2.0","id":1,"method":"initialize"}');
  assert.equal(init.result.protocolVersion, "2024-11-05");
  assert.equal(
    (await handle('{"jsonrpc":"2.0","id":2,"method":"tools/list"}')).result
      .tools.length,
    4,
  );
  assert.deepEqual(
    (await handle('{"jsonrpc":"2.0","id":3,"method":"ping"}')).result,
    {},
  );
  assert.equal(
    await handle('{"jsonrpc":"2.0","method":"notifications/initialized"}'),
    null,
  );
  assert.ok(
    TOOLS.filter((t) =>
      ["aimilia_hora_portugal", "aimilia_noticias_portugal"].includes(t.name),
    ).every((t) => t.annotations.readOnlyHint),
  );
  assert.ok(
    TOOLS.filter((t) =>
      ["aimilia_preferencias", "aimilia_agenda"].includes(t.name),
    ).every(
      (t) => t.annotations.destructiveHint && !t.annotations.readOnlyHint,
    ),
  );
});
test("Lisbon clock respects DST transition and needs no LLM/API", async () => {
  for (const [utc, hour] of [
    ["2026-10-24T12:00:00Z", "13:00"],
    ["2026-10-25T12:00:00Z", "12:00"],
  ]) {
    const handle = createHandler({
      now: () => new Date(utc),
      fetcher: () => {
        throw Error("must not fetch");
      },
    });
    const result = await handle(call("aimilia_hora_portugal"));
    const data = JSON.parse(result.result.content[0].text);
    assert.equal(data.utc, utc.replace("Z", ".000Z"));
    assert.ok(data.local.includes(hour));
  }
});
test("RSS parser bounds output and preserves source/date without HTML", () => {
  const items = parseNews(xml, 1);
  assert.equal(items.length, 1);
  assert.equal(items[0].title, "Notícia & informação");
  assert.equal(items[0].published_at, "2026-10-25T10:00:00.000Z");
  assert.throws(() => parseNews("<rss/>"));
  assert.throws(() => parseNews("x".repeat(524289)));
  assert.throws(() =>
    parseNews(
      "<item><title>evil</title><link>https://rtp.pt.evil.test/a</link></item>",
    ),
  );
});
test("News shares a bounded cache; does not pretend stale content is fresh", async () => {
  let count = 0,
    now = 0;
  const handle = createHandler({
    now: () => new Date(now),
    fetcher: async () => {
      count++;
      return new Response(xml);
    },
  });
  const results = await Promise.all([
    handle(call("aimilia_noticias_portugal", { limit: 1 })),
    handle(call("aimilia_noticias_portugal", { limit: 2 })),
  ]);
  assert.equal(count, 1);
  assert.equal(
    JSON.parse(results[0].result.content[0].text).headlines.length,
    1,
  );
  now = 300001;
  await handle(call("aimilia_noticias_portugal"));
  assert.equal(count, 2);
});
test("Upstream failure is reported honestly and private error strings are not leaked", async () => {
  const handle = createHandler({
    fetcher: async () => {
      throw Error("PRIVATE_TOKEN");
    },
  });
  const result = await handle(call("aimilia_noticias_portugal"));
  assert.equal(result.result.isError, true);
  assert.ok(!JSON.stringify(result).includes("PRIVATE_TOKEN"));
});
test("Invalid tool, arguments, batch, ids and JSON rejected", async () => {
  const handle = createHandler();
  for (const args of [
    { limit: 0 },
    { limit: 6 },
    { limit: 1.5 },
    { limit: "3" },
    { url: "https://internal/" },
  ])
    assert.equal(
      (await handle(call("aimilia_noticias_portugal", args))).error.code,
      -32602,
    );
  assert.equal((await handle(call("unknown"))).error.code, -32602);
  assert.equal(
    (await handle(call("aimilia_hora_portugal", { extra: 1 }))).error.code,
    -32602,
  );
  assert.equal((await handle("[")).error.code, -32700);
  for (const value of [
    "[]",
    "null",
    '{"jsonrpc":"2.0","id":null,"method":"ping"}',
  ])
    assert.equal((await handle(value)).error.code, -32600);
  await assert.rejects(() => handle("x".repeat(32769)));
});
test("Xiaozhi endpoint constrained to expected TLS origin/path", () => {
  assert.equal(
    validateEndpoint("wss://api.xiaozhi.me/mcp/?token=test").hostname,
    "api.xiaozhi.me",
  );
  for (const u of [
    "ws://api.xiaozhi.me/mcp/?token=x",
    "wss://evil.test/mcp/?token=x",
    "wss://api.xiaozhi.me/wrong/?token=x",
    "wss://api.xiaozhi.me/mcp/",
    "wss://user:pass@api.xiaozhi.me/mcp/?token=x",
  ])
    assert.throws(() => validateEndpoint(u));
});
test("Response size guard handles status and content bounds", async () => {
  await assert.rejects(() => boundedText(new Response("bad", { status: 500 })));
  await assert.rejects(() => boundedText(new Response("123456"), 5));
  assert.equal(await boundedText(new Response("Olá")), "Olá");
});

test("Workers-compatible manual redirects reject upstream redirects", async () => {
  let calls = 0;
  const handle = createHandler({
    fetcher: async (_url, options) => {
      calls++;
      assert.equal(options.redirect, "manual");
      return new Response(null, {
        status: 302,
        headers: { Location: "https://other.invalid/" },
      });
    },
  });
  const result = await handle(call("aimilia_noticias_portugal"));
  assert.equal(calls, 1);
  assert.equal(result.result.isError, true);
});
