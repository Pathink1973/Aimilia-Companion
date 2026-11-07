# Aimilia MCP — exemplo para a tua conta

Ponte privada de um único titular: XiaoZhi WebSocket ↔ Cloudflare Durable Object. Notícias RTP, hora, preferências, agenda e Gmail opcional só de leitura.

**[Guia de instalação e privacidade](../../docs/06-mcp-cloudflare.md)**.

```sh
npm ci
npm test
npm run check
```

Nada é publicado por estes três comandos. `npm run deploy` publica na conta Wrangler autenticada. Escolhe o teu nome em `wrangler.jsonc` e fornece Secrets pelo fluxo oficial. Não existem credenciais do autor neste pacote.

Administração: `scripts/control.mjs` exige `AIMILIA_WORKER_URL` e `AIMILIA_ADMIN_TOKEN` do teu ambiente privado. Sem eles recusa o pedido. Não há URL de serviço do autor configurado.

OAuth: `scripts/gmail-authorize.mjs` lê `.secrets/aimilia-gmail-client.json`; o titular autoriza no Google e guarda o payload temporário localmente. Não é uma conta Google partilhada nem autorização que se possa transferir entre makers.

27 testes offline cobrem protocolos, permissões, persistência e Gmail. Eles não provam que uma conta real tenha quota ou autorização ativas.
