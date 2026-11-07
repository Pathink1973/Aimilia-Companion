import test from "node:test";
import assert from "node:assert/strict";
import { gmailPage } from "../src/gmail-pages.js";
test("OAuth identity pages disclose processing and contain no account credentials", () => {
  for (const path of ["/gmail", "/gmail/privacy"]) {
    const page = gmailPage(path);
    assert.ok(page.includes("Aimilia Gmail"));
    assert.ok(!page.includes("<script"));
    assert.ok(!page.includes("GOCSPX"));
    assert.ok(!page.includes("private-owner@example.invalid"));
  }
  assert.ok(gmailPage("/gmail/privacy").includes("XiaoZhi"));
  assert.ok(gmailPage("/gmail/privacy").includes("Cloudflare Secrets"));
  assert.equal(gmailPage("/api/status"), null);
});
