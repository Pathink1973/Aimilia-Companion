# Extensões opcionais: MCP, Cloudflare e Gmail

[← Início](../README.md) · [Privacidade](09-privacidade.md)

A conversa básica não precisa destas extensões. Começa por validar voz e rede. Este capítulo publica uma **ponte de um único titular**, não um serviço multiutilizador. Cada maker cria o próprio Worker, dados, endpoint XiaoZhi e credenciais. Não existe acesso ao serviço privado do autor.

## Ferramentas incluídas

| Ferramenta | O que faz |
| --- | --- |
| `aimilia_hora_portugal` | Data/hora reais em Europe/Lisbon |
| `aimilia_noticias_portugal` | Até cinco manchetes RTP; cache curta e fonte/data |
| `aimilia_preferencias` | Listar, guardar e apagar preferências |
| `aimilia_agenda` | Criar, consultar, alterar, concluir e apagar entradas |
| `aimilia_gmail` | Consulta de Gmail só de leitura, visível apenas após configurar OAuth |

Consulta o código de `src/protocol.js` para esquemas e limites, e `src/personal.js` para persistência. Agenda não dispara alertas automáticos. Preferência de rádio não reproduz áudio. A ponte não fornece navegação web arbitrária nem permite executar código do modelo.

## 1. Preparar o serviço local

Na pasta `integrations/cloudflare-mcp`:

```sh
npm ci
npm test
npm run check
```

Usa Node compatível com a versão de Wrangler fixada em `package-lock.json`. Os testes usam dados falsos e não consultam o teu Gmail. `check` faz um dry-run de build; não publica. Lê `wrangler.jsonc`: nome `aimilia-mcp`, Durable Object `AimiliaBridge`, migração SQLite e cron de recuperação de ligação a cada cinco minutos. Não é um cron de lembretes ao utilizador.

## 2. Criar o teu endpoint e guardar segredos

1. Na consola XiaoZhi, abre o teu agente → Extensions → MCP Endpoint.
2. Cumpre as verificações de conta exigidas pelo serviço (podem mudar; no protótipo foi necessária verificação GitHub).
3. Copia o URL privado `wss://api.xiaozhi.me/mcp/?token=…` **apenas para uma configuração privada**. Não colar em issues, README ou prompts públicos.
4. Autentica Wrangler na tua conta Cloudflare por `npx wrangler login`, revendo os pedidos oficiais.
5. Escolhe um nome de Worker que não colida com os teus projetos. Se usares o nome sugerido, todos os comandos abaixo devem usar essa mesma configuração.
6. Usa os prompts interativos de Secrets, que não guardam os valores no código:

```sh
npx wrangler secret put MCP_ENDPOINT
npx wrangler secret put ADMIN_TOKEN
```

Cria um `ADMIN_TOKEN` aleatório de pelo menos 32 caracteres (por exemplo 32 bytes codificados em hexadecimal) e guarda-o num gestor de segredos privado. Não usar um token incluído num exemplo. Não colocar secrets em `vars` no `wrangler.jsonc`.

## 3. Publicar e ativar

```sh
npm run deploy
```

O deploy é feito **na tua conta**. Confirma o plano, limites e condições do fornecedor. Não há promessa de custo zero ilimitado. Guarda o URL HTTPS do Worker apresentado por Wrangler.

O script de administração lê duas variáveis **do teu terminal privado**: `AIMILIA_WORKER_URL` (origem HTTPS do teu Worker) e `AIMILIA_ADMIN_TOKEN`. Define-as localmente sem escrever o token no histórico; podes pedir ao Codex que prepare um método de entrada segura. Não há uma origem remota pré-preenchida que contacte o autor.

```sh
node scripts/control.mjs status
node scripts/control.mjs start
```

`start` abre um ensaio de 15 minutos. Depois de validar descoberta e chamadas reais:

```sh
node scripts/control.mjs enable
```

`enable` mantém a ligação enquanto não a parares. Usa `node scripts/control.mjs stop` para parar; a paragem persiste e o cron não a desfaz. `status` mostra estado e contadores limitados, sem expor o endpoint. Não mudar o identificador interno `aimilia-service-v1` após guardar dados sem planear migração.

## 4. Validar no XiaoZhi e na placa

- A consola deve indicar Online e descobrir as ferramentas.
- Reinicia a sessão da placa depois de alterar permissões ou descoberta.
- Pede: «Consulta uma manchete atual da RTP usando aimilia_noticias_portugal».
- Pede: «Guarda a Antena 1 como minha rádio favorita» e «Qual é a minha rádio favorita?».
- Cria uma entrada de teste com data inequívoca, consulta e pede para a apagar.
- Confere os contadores de chamada. Uma resposta à hora, por si só, não prova que o MCP foi usado.

Se o modelo disser que não tem ferramentas, investigar descoberta, permissões da conta e renovação de sessão. Se há chamada mas falha a fonte, investigar a rede/origem e os erros limitados. Não remover autenticação para «resolver» ligação.

## 5. Gmail opcional — só depois das ferramentas básicas

Não é necessário para companhia. É sensível: emails consultados podem passar por Google, Cloudflare e XiaoZhi para produzir respostas e podem ser ouvidos por quem estiver perto. A voz não autentica o titular.

1. Cria um projeto Google Cloud próprio e ativa Gmail API.
2. Configura o ecrã de consentimento e um cliente OAuth do tipo **Web application**, com retorno exato `http://localhost:8768/oauth/callback`.
3. Pede **apenas** `https://www.googleapis.com/auth/gmail.readonly`. Não pedir envio, alteração ou acesso total.
4. Guarda o JSON do cliente em `integrations/cloudflare-mcp/.secrets/aimilia-gmail-client.json`, com pasta privada e permissões de ficheiro 0600 em macOS/Linux. `.secrets` está ignorada pelo Git.
5. Executa `node scripts/gmail-authorize.mjs`. Abre a página local indicada e o titular aprova a autorização Google pessoalmente. Lê os avisos; não pressupor que «Em produção» significa aplicação verificada ou token eterno.
6. O script guarda um payload privado temporário com `GMAIL_CLIENT_ID`, `GMAIL_CLIENT_SECRET` e `GMAIL_REFRESH_TOKEN`. Instala esses valores no teu Worker com `wrangler secret put` ou `secret bulk` apontado **ao ficheiro privado indicado pelo script**, sem os imprimir. Confere o formato e o nome do ficheiro antes de executar; nunca colar o JSON no chat.
7. Remove o ficheiro temporário de transferência depois de validar a instalação, mantendo apenas o necessário para a tua renovação em armazenamento seguro.
8. Reinicia a ligação da ponte e executa `node scripts/control.mjs gmail-check`. O diagnóstico não deve imprimir conteúdo das mensagens.
9. Confirma a descoberta de `aimilia_gmail` e testa uma consulta por voz.

No modo de teste, as autorizações Google com estes escopos podem expirar ao fim de sete dias. A passagem a produção, verificação, quotas e exigências dependem do Google e do tipo de utilização; não garantem autorização permanente. Um `invalid_grant` pode exigir nova autorização do titular. Usa sempre as [regras atuais OAuth Google](https://developers.google.com/identity/protocols/oauth2#expiration) e [escopos Gmail](https://developers.google.com/workspace/gmail/api/auth/scopes).

As páginas `/gmail` e `/gmail/privacy` fornecidas são um modelo informativo para instalação pessoal. Revê-as para corresponderem à tua implementação e responsável antes de as usares numa configuração OAuth ou num serviço para terceiros. Este pacote não cria um serviço público de Gmail para outros utilizadores.

## Persistência, segurança e manutenção

- `/health` é público; rotas de administração exigem Bearer privado.
- Agenda/preferências são guardadas no Durable Object, não no Git nem automaticamente no ecrã.
- Segredos não são guardados no armazenamento de preferências.
- Não ativar logs de corpos de emails ou tokens. Redigir logs antes de pedir ajuda.
- Rotação e revogação exigem atualização da ligação; confirma a instância efetivamente em execução, não apenas um processo novo.
- Limites de conteúdo e tratamento de emails como dados externos reduzem riscos, mas não tornam um LLM imune a instruções maliciosas contidas em emails.

Fontes: [Durable Objects](https://developers.cloudflare.com/durable-objects/get-started/), [Cloudflare Secrets](https://developers.cloudflare.com/workers/configuration/secrets/), [escopos Gmail](https://developers.google.com/workspace/gmail/api/auth/scopes). Código com testes incluído em [integrations/cloudflare-mcp](../integrations/cloudflare-mcp/).
