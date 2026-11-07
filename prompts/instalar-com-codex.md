# Prompt para instalar a Aimilia com o Codex

Copia o bloco abaixo para o Codex, com este repositório aberto numa pasta local. Não incluir palavras-passe ou tokens no pedido. O objetivo é instalar com inspeção e validação, não autorizar antecipadamente qualquer apagamento.

```text
Quero construir e instalar a Aimilia deste repositório na minha Waveshare
ESP32-S3-Touch-AMOLED-1.75C (variante C, 466×466 AMOLED, 8 MB PSRAM,
32 MB flash). Trabalha como engenheiro de firmware ESP-IDF, preservando
outros projetos, configurações pessoais e a possibilidade de recuperação.

Lê primeiro README.md, AGENTS.md, firmware/manifest.json,
docs/03-instalacao.md, docs/08-validacao.md e advanced/dualboot/README.md.
Não assumes que a minha placa é igual ao protótipo privado do autor.

1. AUDITORIA SEM GRAVAÇÃO
- Identifica SO, Git/Python/ESP-IDF, espaço livre e porta USB.
- Confirma chip, tamanho de flash e estado de segurança por operações
  de leitura. Não queimar eFuses, desativar segurança ou apagar flash.
- Pergunta se a placa já tem Muse/dual boot ou firmware a preservar.
- Se tiver dual boot, não usar a tabela standalone. Inventaria partições
  e prepara um plano específico; não executar install.py de outro projeto.
- Não ler ficheiros .secrets, conversas ou contas que não sejam necessários.

2. PREPARAÇÃO ISOLADA
- Usa o commit XiaoZhi fixado no manifest, não a última main.
- Executa scripts/prepare.py numa pasta limpa; verifica hashes do overlay.
- Mantém o repositório original e trabalho não relacionado intactos.
- Ativa ESP-IDF 6.0.1. Confirma a versão antes de compilar.
- Usa scripts/build.py, perfil esp32-s3-touch-amoled-1.75c,
  idioma de menus pt-PT e wake-word disabled como documentado.
- Não alterar pinagem, protocolo ou tarefas de áudio sem necessidade.

3. BACKUP E PLANO CONCRETO
- Faz um backup completo da minha própria flash para uma pasta privada,
  verifica tamanho e SHA-256. Nunca colocar dumps ou NVS no Git.
- Antes da gravação, apresenta variante, SDK, tabela, offsets, ficheiros,
  tamanhos e verificação de que cada imagem cabe na sua partição.
- Explica o impacto sobre o firmware existente. Pede a minha autorização
  para esse plano concreto. Não confundir autorização de build com flash.
- Não usar erase-flash, offsets copiados de tutoriais, binários de outra
  pessoa, redes predefinidas ou chaves do autor.

4. INSTALAÇÃO E CONFIGURAÇÃO
- Só depois de autorizado, grava os ficheiros compilados para o layout
  validado, usando os argumentos do próprio build.
- Orienta-me a configurar o meu Wi-Fi 2,4 GHz e a associar o código de
  ativação à minha conta XiaoZhi. Eu trato de login e consentimentos.
- Usa prompts/personalidade-aimilia.md no meu agente, dentro dos limites
  da consola. Não muda modelo ou ferramentas sem explicar a necessidade.
- Explica que a voz portuguesa testada é pt-BR; pt-PT no texto não muda
  o sotaque e não há garantia de uma voz portuguesa europeia.

5. VALIDAÇÃO FÍSICA
- Testa arranque, captura, uma resposta curta e uma longa até ao fim,
  boca, 18 poses, três skins, toque, movimento, menu, volume, brilho,
  redes e bateria. Não alegar testes de hardware sem observar resultados.
- Investiga áudio cortado com logs redigidos e estado de reprodução;
  não inventar que se deve apenas ao modelo.
- No hotspot iPhone, verificar Maximizar compatibilidade e 2,4 GHz.
- Confirma reinício e funcionamento sem o Mac ligado.
- Se falhar, diagnostica antes de voltar a gravar e preserva o backup.

6. EXTENSÕES, APENAS SE EU QUISER
- Primeiro conclui a conversa básica.
- Usa docs/06-mcp-cloudflare.md para o meu Worker e endpoint privados.
- Nunca reutilizar o serviço do autor ou colocar segredos no código/chat.
- Gmail só com o meu consentimento OAuth e gmail.readonly; não ultrapasses
  avisos de segurança por mim. Agenda não dispara alarmes automáticos.

No final, entrega um relatório com o que foi compilado, gravado e testado,
versões e hashes, limitações reais e instruções de recuperação específicas
para a minha placa. Não publicar credenciais nem o meu backup.
```

Para uma análise sem instalação, acrescenta: **«Nesta sessão não autorizo escrever na placa; prepara apenas fontes, build e relatório.»**
