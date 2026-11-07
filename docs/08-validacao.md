# Registo de validação da edição pública

[← Início](../README.md)

Publicação inicial: **7 de novembro de 2026**. Resultados dizem respeito aos ficheiros e revisão fixados em `firmware/manifest.json`; não certificam qualquer alteração futura.

## Proveniência

- Base: XiaoZhi `cdc74a0c1467ea517b37b44a8d56d517c715f6c8`.
- Customizações: ficheiros fonte exportados por lista permitida, sem histórico privado, builds ou NVS.
- Perfil público: standalone 1.75C, ESP-IDF 6.0.1, textos pt-PT, palavra de ativação desativada.
- Fonte de dados MCP: código e testes isolados; configuração de publicação sem conta, hostname privado ou tokens do autor.
- Sons locais públicos: sinais sintéticos não vocais originais, diferentes das vozes do protótipo.

## Verificações desta publicação

O resultado final dos testes é registado em `validation.json` na raiz. Inclui testes de consistência, fontes/manifest, preparação isolada, build de firmware quando concluído e testes offline de MCP.

Os testes de firmware em host exercitam geometria superelíptica, contexto local, contadores de áudio, renderização Formas, e bateria; a compilação inclui a restauração de superfícies. Não exercitam o hardware real. O build verifica compilação/link e limites de imagem no perfil standalone.

O protótipo privado já foi usado fisicamente pelo autor com voz, expressões, menu, Wi-Fi e ferramentas. **A edição pública standalone não foi gravada nesta publicação na placa do autor**, para preservar a instalação que está funcional. Não há confirmação física independente de uma placa nova com este perfil. A validação física do maker segue [a checklist](07-testes-e-diagnostico.md).

## Diferenças a considerar

- Standalone não inclui Muse nem seletor de arranque no fluxo principal.
- O duplo clique no perfil standalone tem o comportamento definido no código AEC, em vez do retorno ao menu dual boot.
- As contas, voz, memória e ferramentas dependem da configuração de cada utilizador.
- A instalação limpa de terceiros, Windows, autonomia da bateria e outros modelos de Waveshare não são declarados validados.
- A sincronização da boca é baseada no sinal de áudio; não é reconhecimento de fonemas.

Não disponibilizamos binários ou um rótulo de «release estável para qualquer placa» sem ensaio físico específico. Usa a versão fonte, preserva um backup e reporta resultados reproduzíveis.
