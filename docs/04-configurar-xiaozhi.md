# Configurar o agente XiaoZhi

[← Instalação](03-instalacao.md) · [Interface →](05-interface-e-expressoes.md)

## Rede e ativação

1. Abre [xiaozhi.me](https://xiaozhi.me/) e entra com a tua própria conta. A consola pode iniciar noutra língua; usa a opção de idioma da interface quando disponível.
2. Liga a placa e segue o método de configuração de rede indicado no seu ecrã. O nome do ponto de acesso ou portal pode variar com o firmware; não existe uma palavra-passe universal neste guia.
3. Usa Wi-Fi de 2,4 GHz, sem portal cativo. Configura as redes que realmente pretendes usar.
4. Quando surgir o código de ativação, associa-o ao teu agente na [consola](https://xiaozhi.me/console/agents). Não o publicar.
5. Dá ao agente o nome **Aimilia**. Confirma que a sessão e as permissões ficam ativas após reiniciar a placa.

Num iPhone, ativa «Permitir acesso a terceiros» e **«Maximizar compatibilidade»** para o hotspot. Mantém o ecrã do hotspot aberto durante a primeira ligação. Guarda SSID e palavra-passe na configuração privada do dispositivo, nunca no código ou num pedido público.

## Personalidade

Copia [prompts/personalidade-aimilia.md](../prompts/personalidade-aimilia.md) para **Role Introduction**, respeitando o limite de caracteres/tokens da consola. É um modelo público sem dados pessoais. Ajusta o nome do interlocutor e preferências apenas na tua conta se quiseres.

- Define uma companheira acolhedora, curiosa e respeitadora.
- Respostas curtas ajudam a experiência por voz, mas não garantem menor latência de rede.
- O modo criativo ajuda a refletir; o modo companhia acompanha conversas.
- Não é necessário perguntar o modo em cada interação; o agente deve inferir pelo contexto e clarificar só quando fizer diferença.
- O texto do papel não instala ferramentas nem cria alarmes. Mantém as instruções alinhadas com o que configuraste.

## Modelo, voz e idioma

Em **Model & memory** ou secção equivalente, verifica as opções disponíveis na tua conta. Usa inicialmente a configuração que funciona e compara alterações com as mesmas perguntas. Não há um modelo ou plano pago imposto pelo projeto.

No protótipo a voz portuguesa disponível e usada é **pt-BR**. Os menus e instruções podem estar em pt-PT; o sotaque depende do sintetizador selecionado. Não prometemos pt-PT nativo, uma voz específica ou voz ElevenLabs sem integração própria. Esta publicação não precisa de uma conta ElevenLabs.

O perfil de build tem `--wake-word disabled`: usa o botão de conversa. A ativação por palavra pode ser experimentada depois com suporte upstream e testes de captura, mas não é um requisito para a Aimilia.

## Memória, serviços e reconhecimento de locutor

A memória do serviço pode ajudar a dar contexto, mas pode falhar. Revê o que está guardado na tua consola e apaga informação indevida. Não presumir que a memória do modelo é a base de dados de agenda no Cloudflare: são camadas distintas.

Speaker Recognition, quando disponível, distingue locutores configurados. Não usar como autenticação fiável para dados pessoais. No protótipo só o autor usava a placa; numa casa partilhada outras pessoas podem ouvir as respostas e pedir consultas.

As extensões Weather/Music do serviço não equivalem a IPMA/RTP ou a catálogo mundial de música. Evita ferramentas com nomes concorrentes que confundam o modelo. [As ferramentas MCP próprias são opcionais e descritas aqui](06-mcp-cloudflare.md).

## Verificar a primeira sessão

- «Olá, quem és?» — nome e tom corretos.
- «Explica-me em duas frases o que é a fotossíntese.» — ouvir até ao fim.
- «Hoje estou contente com o meu projeto.» — expressão adequada, sem exigir que apareça sempre uma emoção específica.
- «Que tempo faz em Lisboa? Usa a ferramenta de meteorologia disponível.» — distinguir consulta real de uma resposta do modelo sem ferramenta.

Regista ferramenta usada, resultado e falhas. Nunca afirmar que uma resposta está atualizada apenas porque soou convincente.
