# Contribuir para a Aimilia

Obrigado pelo interesse. Este é um projeto maker em evolução, com foco em presença visual, conversa fluida e instruções reproduzíveis.

## Antes de alterar

Lê o README, a licença e o estado de validação. Abre uma issue para mudanças grandes. Não alterar a revisão upstream, a pinagem, as partições ou o protocolo na mesma alteração que um ajuste cosmético.

## Ambiente

- Firmware: revisão e SDK em `firmware/manifest.json`.
- `python3 scripts/prepare.py` cria uma árvore isolada.
- `python3 scripts/build.py` compila sem flash.
- `python3 scripts/test_firmware.py` executa testes host dos ficheiros públicos.
- MCP: `npm ci && npm test && npm run check` na pasta da integração.
- `python3 scripts/verify_public.py` revê links locais, hashes e padrões proibidos.

Se alterares um ficheiro em `firmware/overlay`, atualiza o respetivo SHA-256 no manifest e explica porquê. Não modificar ficheiros gerados como solução permanente.

## Pull requests

Explica o problema, o comportamento esperado e o que foi testado. Para firmware, separa compilação de ensaio físico e indica placa, SDK e layout. Para áudio, testa uma resposta curta e longa, interrupção, eco e reconexão. Para UI, inclui imagens sem dados pessoais e verifica legibilidade e memória.

Não publicar logs completos, `.secrets`, `.dev.vars`, NVS, dumps ou binários com dados pessoais. Não adicionar telemetria, contas partilhadas ou serviços remotos do autor como dependências.

Respeita atribuições. Alterações derivadas de terceiros devem incluir a sua licença e origem. A comunidade não deve apresentar a Aimilia como produto médico ou prometer funcionalidades não implementadas.
