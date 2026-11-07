import test from "node:test";
import assert from "node:assert/strict";
import { personalService, parseAppointment } from "../src/personal.js";
import { createHandler } from "../src/protocol.js";
function database() {
  let data = new Map(),
    queue = Promise.resolve();
  return {
    snapshot: () => structuredClone(data),
    transaction(fn) {
      const operation = queue.then(async () => {
        const draft = structuredClone(data);
        const result = await fn({
          get: async (k) => structuredClone(draft.get(k)),
          put: async (k, v) => draft.set(k, structuredClone(v)),
        });
        data = draft;
        return result;
      });
      queue = operation.catch(() => {});
      return operation;
    },
  };
}
const clock = () => new Date("2026-10-25T17:00:00Z");
test("Preferences survive service recreation; explicit writes, deletion and unsafe keys", async () => {
  const db = database();
  let use = personalService(db, clock);
  await assert.rejects(
    use("aimilia_preferencias", {
      action: "guardar",
      chave: "localidade",
      valor: "Porto",
    }),
  );
  await use("aimilia_preferencias", {
    action: "guardar",
    chave: "localidade",
    valor: "Porto",
    confirmado: true,
  });
  use = personalService(db, clock);
  assert.equal(
    (await use("aimilia_preferencias", { action: "listar" })).preferencias[0]
      .valor,
    "Porto",
  );
  for (const key of ["constructor", "__proto__", "prototype"])
    await assert.rejects(
      use("aimilia_preferencias", {
        action: "guardar",
        chave: key,
        valor: "x",
        confirmado: true,
      }),
    );
  await use("aimilia_preferencias", {
    action: "apagar",
    chave: "localidade",
    confirmado: true,
  });
  assert.equal(
    (await use("aimilia_preferencias", { action: "listar" })).preferencias
      .length,
    0,
  );
});
test("Dates require an offset, reject impossible dates and preserve DST offsets", () => {
  for (const at of [
    "amanhã",
    "2026-10-26T09:00:00",
    "2026-02-30T09:00:00Z",
    "2026-10-26T25:00:00Z",
    "2026-10-26T09:00:00+15:00",
  ])
    assert.throws(() => parseAppointment(at));
  assert.equal(
    parseAppointment("2026-10-25T01:30:00+01:00"),
    "2026-10-25T00:30:00.000Z",
  );
  assert.equal(
    parseAppointment("2026-10-25T01:30:00+00:00"),
    "2026-10-25T01:30:00.000Z",
  );
});
test("Agenda CRUD, duplicate retry and concurrent writes survive reconstruction", async () => {
  const db = database();
  let count = 0;
  let use = personalService(db, clock, () => String(++count));
  const create = {
    action: "criar",
    titulo: "Teste",
    tipo: "lembrete",
    data_hora: "2026-10-26T09:00:00Z",
    confirmado: true,
  };
  const results = await Promise.all([
    use("aimilia_agenda", create),
    use("aimilia_agenda", create),
    use("aimilia_agenda", { ...create, titulo: "Segundo" }),
  ]);
  assert.equal(results[1].ja_existia, true);
  use = personalService(db, clock);
  let list = await use("aimilia_agenda", { action: "listar" });
  assert.equal(list.entradas.length, 2);
  assert.equal(list.avisos_automaticos, false);
  const id = results[0].entrada.id;
  await use("aimilia_agenda", {
    action: "alterar",
    id,
    titulo: "Alterado",
    confirmado: true,
  });
  await use("aimilia_agenda", { action: "concluir", id, confirmado: true });
  assert.equal(
    (await use("aimilia_agenda", { action: "listar" })).entradas.length,
    1,
  );
  await use("aimilia_agenda", { action: "apagar", id, confirmado: true });
  assert.equal(
    (await use("aimilia_agenda", { action: "listar", filtro: "todos" }))
      .entradas.length,
    1,
  );
});
test("Failed updates roll back, due items are queries and writes require permission", async () => {
  const db = database(),
    use = personalService(db, clock, () => "fixed");
  await use("aimilia_agenda", {
    action: "criar",
    titulo: "Passado",
    data_hora: "2026-10-25T09:00:00Z",
    confirmado: true,
  });
  await assert.rejects(
    use("aimilia_agenda", {
      action: "alterar",
      id: "fixed",
      titulo: "Não deve gravar",
      data_hora: "invalida",
      confirmado: true,
    }),
  );
  const list = await use("aimilia_agenda", {
    action: "listar",
    filtro: "vencidos",
  });
  assert.equal(list.entradas[0].titulo, "Passado");
  await assert.rejects(
    use("aimilia_agenda", { action: "apagar", id: "fixed" }),
  );
  await assert.rejects(
    use("aimilia_agenda", { action: "listar", extra: "injection" }),
  );
});
test("MCP hides storage failures and does not claim an automatic reminder", async () => {
  const use = personalService(
    {
      transaction: async () => {
        throw Error("PRIVATE infrastructure error");
      },
    },
    clock,
  );
  const handle = createHandler({ personal: use });
  const r = await handle(
    JSON.stringify({
      jsonrpc: "2.0",
      id: 1,
      method: "tools/call",
      params: { name: "aimilia_agenda", arguments: { action: "listar" } },
    }),
  );
  assert.equal(r.result.isError, true);
  assert.ok(!JSON.stringify(r).includes("PRIVATE"));
});

test('Storage limits and paginated agenda do not discard existing records', async()=>{
 const db=database(),use=personalService(db,clock,()=>crypto.randomUUID());
 for(let i=0;i<32;i++) await use('aimilia_preferencias',{action:'guardar',chave:'p'+i,valor:'valor',confirmado:true});
 await assert.rejects(use('aimilia_preferencias',{action:'guardar',chave:'overflow',valor:'valor',confirmado:true}));
 assert.equal((await use('aimilia_preferencias',{action:'listar'})).preferencias.length,32);
 for(let i=0;i<23;i++) await use('aimilia_agenda',{action:'criar',titulo:'Evento '+i,data_hora:'2026-10-26T10:00:00Z',confirmado:true});
 const first=await use('aimilia_agenda',{action:'listar'}),second=await use('aimilia_agenda',{action:'listar',offset:20});
 assert.equal(first.entradas.length,20);assert.equal(first.total,23);assert.equal(first.proximo_offset,20);
 assert.equal(second.entradas.length,3);assert.equal(second.proximo_offset,null);
});
