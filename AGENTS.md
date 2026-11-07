# Aimilia — instruções para agentes de desenvolvimento

Este repositório é público. Preserva credenciais, estado do dispositivo e trabalho de outras pastas.

- Lê README.md, firmware/manifest.json e docs/03-instalacao.md antes de trabalhar no firmware.
- Trabalha apenas em work/xiaozhi preparado pelo script; não alterar a instalação privada do autor.
- A revisão upstream é fixa. ESP-IDF testado 6.0.1; não atualizar SDK/base silenciosamente.
- prepare/build/test não autorizam flash. Antes de escrever numa placa, identificar modelo, segurança e tabela, verificar backup privado e apresentar um plano para aprovação do titular.
- Nunca executar erase-flash, alterar eFuses ou desativar proteções para fazer uma receita funcionar.
- Não misturar standalone com dual boot. O seletor avançado não é um instalador automático.
- Nunca ler/copiar .secrets, .dev.vars, backups, NVS ou contas do autor para o repositório.
- Credenciais devem ser entradas em fluxos privados do fornecedor; não pedir que sejam coladas no chat.
- Preserva áudio e callbacks não bloqueantes. Após mudar UI, testar voz até ao fim e memória; após alterar áudio, testar eco/interrupção e reconexão.
- Guarda alterações de firmware no overlay público, atualiza os hashes no manifest e recompila a variante afetada. Não editar apenas ficheiros gerados de work/build.
- Valida scripts/verify_public.py, scripts/test_firmware.py e os testes MCP se afetados.
- Não afirmar validação física quando só houve compilação. Atualizar docs/08-validacao.md com limites reais.
- Notícias, agenda, preferências e Gmail são opcionais em contas próprias; agenda não toca alarmes e Gmail é só leitura.
