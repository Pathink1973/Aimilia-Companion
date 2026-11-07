# Segurança

Não coloques vulnerabilidades com credenciais ou dados pessoais numa issue pública. Usa a opção GitHub **Security → Report a vulnerability**, quando disponível, para comunicar de forma privada. Na ausência dessa opção, abre apenas uma issue genérica a pedir um canal privado, sem detalhes exploráveis ou segredos.

Se expuseste um token, revoga-o no fornecedor e substitui-o. Apagar o ficheiro do último commit não remove um segredo do histórico.

Não existe garantia de atualizações de segurança ou SLA. Cada titular deve manter dependências, contas e autorizações e rever o que envia aos serviços externos. A ponte MCP é de um único titular e não deve ser apresentada como serviço multiutilizador sem desenho de autenticação, isolamento e consentimento próprios.

Regras de contribuição: nunca distribuir backups, NVS, passwords Wi-Fi, clientes OAuth, chaves privadas, tokens, logs pessoais ou endpoints MCP autenticados.
