# Privacidade e dados

[← Início](../README.md) · [Segurança](../SECURITY.md)

## Onde passam os dados

- O microfone capta voz para a conversa e o serviço XiaoZhi processa áudio, texto, modelo e síntese conforme a sua configuração e políticas.
- Agenda e preferências opcionais ficam no Durable Object da conta Cloudflare do próprio maker.
- Gmail é opcional. Requer consentimento OAuth do titular; as mensagens consultadas podem passar por Google, Worker e XiaoZhi para produzir a resposta.
- O website de apresentação é separado: não se liga ao teu agente, à tua placa ou ao Gmail.

Não é uma solução integralmente local. Código open source não substitui a leitura das políticas dos fornecedores. Não há garantia de residência de dados, retenção zero ou imunidade a pedidos maliciosos em conteúdo externo.

## O que não publicar

Palavras-passe, endpoint MCP com token, cliente OAuth, refresh token, chaves de assinatura, dumps, NVS, logs com rede/códigos de ativação, emails, memórias e compromissos privados. As pastas de trabalho e segredos estão ignoradas por `.gitignore`, mas esse ficheiro não é uma barreira de segurança: revê cada commit antes de publicar.

## Revogar

- Gmail: retirar a ligação na [Conta Google](https://myaccount.google.com/connections), remover Secrets do Worker e terminar a ligação da ponte.
- MCP: parar a ponte, revogar/substituir endpoint na consola quando suportado e rever tokens Cloudflare.
- Dados de agenda/preferências: usar as operações de eliminação das ferramentas e gerir a instância Cloudflare. Retirar um token não apaga dados persistidos.
- Memória e históricos XiaoZhi: gerir no serviço respetivo. Não são apagados automaticamente quando o Worker pára.

Se a placa for usada por mais pessoas, elas podem ouvir respostas ou solicitar informação. Não existe autenticação biométrica fiável neste projeto. Para dados sensíveis, limita ferramentas e pondera o local de utilização.
