# Hardware e material necessário

[← Início](../README.md) · [Seguinte: instalação →](03-instalacao.md)

## Placa exata

**Waveshare ESP32-S3-Touch-AMOLED-1.75C**. O sufixo **C** identifica a variante usada. Uma placa com outro tamanho, controlador ou pinagem exige adaptação e validação; não selecionar automaticamente o perfil de 1.75 sem C.

| Elemento | Referência |
| --- | --- |
| Microcontrolador | ESP32-S3R8, Xtensa LX7 de dois núcleos, até 240 MHz |
| RAM externa / flash | 8 MB PSRAM / 32 MB flash NOR |
| Ecrã | AMOLED circular 1,75″, 466 × 466 píxeis; ligação QSPI |
| Controlador / toque | CO5300 / CST9217 capacitivo |
| Rádio | Wi-Fi 2,4 GHz 802.11 b/g/n; Bluetooth LE disponível no hardware |
| Áudio | Dois microfones, ES7210 e codec ES8311, altifalante integrado |
| Movimento | IMU QMI8658, acelerómetro e giroscópio de seis eixos |
| Energia | PMIC AXP2101, USB-C e bateria compatível |
| Caixa | Liga de alumínio; cerca de Ø 55 mm × 15,05 mm |
| Botões | PWR e BOOT; funções dependem do firmware |

A presença de Bluetooth no hardware não significa que a Aimilia implemente chamadas ou auscultadores Bluetooth. Cancelamento de eco é uma configuração de áudio, não uma garantia de ausência de cortes em qualquer ambiente. A autonomia depende de brilho, rede, uso e bateria e não é anunciada neste projeto.

## Comprar em Portugal

**[Mauser — Waveshare 33691, referência 095-9590](https://mauser.pt/095-9590/waveshare-33691-kit-desenvolvimento-esp32-s3-com-ecra-amoled-1-75)**.

A página identifica o modelo 1.75C. Confirma no momento da compra a variante, disponibilidade, preço, bateria e restantes componentes incluídos. Não há preço fixo neste guia e não garantimos stock. A ligação é de referência, não um checkout ou promessa de venda da Aimilia.

## Lista de material

- Placa 1.75C e bateria adequada fornecida ou especificada para a placa.
- Cabo USB-C **com dados** (alguns cabos apenas carregam).
- Mac, Linux ou Windows com ferramentas de desenvolvimento; terminal e espaço suficiente para ESP-IDF, componentes e builds. Recomenda-se reservar pelo menos 10 GB livres como margem prática, não um requisito medido.
- Rede Wi-Fi **2,4 GHz** e acesso à Internet para a conversa.
- Conta XiaoZhi própria. Cloudflare e Google são opcionais para as extensões.

Não é necessário soldar para a configuração normal desta placa. Não alterar ligações, polaridade da bateria ou PMIC para seguir este guia.

## Fontes e conferência

- [Documentação oficial Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C).
- [Produto oficial, SKU 33691](https://www.waveshare.com/esp32-s3-touch-amoled-1.75c.htm?sku=33691).
- [Página Mauser](https://mauser.pt/095-9590/waveshare-33691-kit-desenvolvimento-esp32-s3-com-ecra-amoled-1-75).
- Pinagem de compilação: `main/boards/waveshare/esp32-s3-touch-amoled-1.75/config.h` do upstream fixado. Não a alterar para contornar incompatibilidades de outra placa.

Conferido em 7 de novembro de 2026. Características de fornecedor podem mudar; verifica o modelo efetivamente recebido.
