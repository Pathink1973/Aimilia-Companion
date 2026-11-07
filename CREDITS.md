# Créditos, autoria e licenças

## Aimilia

**Patrício Brito** — conceito, direção criativa, identidade, personalização da interface, integração de funcionalidades e construção do protótipo.

**[Academia crIA · www.academiacria.pt](https://www.academiacria.pt/)**.

Website: **[aimilia.vercel.app](https://aimilia.vercel.app/)**. Desenvolvimento assistido por ferramentas de IA, incluindo Codex. Capa ilustrativa criada com Imagegen a partir da identidade do site. Os sinais locais públicos são sons sintéticos originais não vocais.

Código/documentação próprios disponibilizados sob [MIT](LICENSE). A licença do repositório não substitui as licenças de dependências ou as condições dos serviços e vozes de terceiros. Não são distribuídas as antigas vozes de sistema locais, o áudio ElevenLabs do autor, chaves ou binários Muse.

## Componentes e referências

| Projeto / entidade | Utilização e atribuição | Licença / ligação |
| --- | --- | --- |
| [XiaoZhi / 78](https://github.com/78/xiaozhi-esp32) | Base do firmware, áudio, rede, LVGL, protocolo e agentes; ficheiros modificados no overlay conservam a origem | [MIT upstream preservada](licenses/XIAOZHI-MIT.txt) |
| [Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C) | Hardware de referência e respetiva documentação | Marcas e documentação dos respetivos titulares; não vendemos hardware |
| [Espressif ESP-IDF](https://github.com/espressif/esp-idf) | SDK de compilação; não incluído no repositório | Licenças do SDK e seus componentes |
| [LVGL](https://github.com/lvgl/lvgl) | Interface embarcada, obtida pelo gestor de componentes do upstream | Licença no componente obtido |
| [esp32-eyes / playfultechnology](https://github.com/playfultechnology/esp32-eyes) | Referência conceptual para olhos expressivos; geometria Aimilia desenhada em LVGL, sem renderer Arduino copiado | Sem código/bitmap desse projeto distribuído como pacote separado |
| [UNIT-Electronics-MX / Kubi](https://github.com/UNIT-Electronics-MX/unit_kubi_pulsar_desk_pet) | Adaptação não bloqueante de comportamentos de repouso, despertar e entusiasmo em kubi_animation.h | [MIT preservada](licenses/KUBI-MIT.txt), copyright 2025 UNIT-Electronics-MX |
| LVGL Kawaii Face | Investigação e experiências visuais anteriores do projeto; biblioteca não incluída nesta edição pública | Não alegamos distribuir nem incorporar o renderer original neste pacote |
| [IPMA](https://api.ipma.pt/) | Fonte de meteorologia consultada pela ferramenta | Serviço e dados externos; não duplicados neste repositório |
| [RTP](https://www.rtp.pt/noticias/) | Fonte de manchetes da extensão | Conteúdo não incluído no repositório; consultado em execução |
| [Cloudflare](https://developers.cloudflare.com/) / [Google](https://developers.google.com/workspace/gmail/api/) | Plataformas opcionais usadas em contas próprias | Condições e autorizações dos respetivos serviços |

A capa e os modelos do website são ilustrações do dispositivo, não fotografias oficiais nem CAD do fabricante. Aimilia é independente e não representa Waveshare, XiaoZhi, Espressif, Mauser, Google ou Cloudflare.

## Dependências obtidas no build

O `firmware/dependencies.lock` fixa versões e hashes de componentes públicos do build. Cada componente traz a sua licença; o responsável por redistribuir um firmware deve preservar os avisos aplicáveis, incluindo fontes, modelos e assets obtidos. Não se deve assumir que a licença MIT da Aimilia autoriza qualquer uso de vozes, serviços ou marcas externas.
