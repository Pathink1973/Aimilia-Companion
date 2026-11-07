# Construir e instalar a Aimilia, passo a passo

[← Início](../README.md) · [Configurar o XiaoZhi →](04-configurar-xiaozhi.md)

## 0. Escolhe o caminho certo antes de ligar ferramentas de gravação

| Situação da tua placa | Caminho |
| --- | --- |
| Nova, ou aceitas substituir o firmware existente depois de teres backup | Perfil **standalone** deste guia |
| Já tem Muse + XiaoZhi + menu de arranque | **Não executar o flash standalone**. Ler [dual boot avançado](../advanced/dualboot/README.md) |
| Firmware desconhecido, variante diferente, segurança de flash desconhecida | Identificar primeiro. Não adivinhar offsets nem apagar |

O perfil standalone usa a tabela upstream `partitions/v2/16m.csv` numa flash física de **32 MB**: NVS, OTA e 8 MB de assets ocupam a zona inferior de 16 MB; o restante espaço não é usado. Não é a tabela do protótipo dual boot. O offset da tabela standalone é `0x8000`; o dual boot de referência usa `0x10000`. **Misturar estas tabelas ou os seus binários pode impedir o arranque.**

Este guia não inclui `erase-flash`, alterações de eFuses, desativação de secure boot ou reutilização das chaves do autor. Se a placa tiver secure boot/flash encryption ativados, não a tentar converter com esta receita: é necessária uma análise compatível com a segurança dessa placa.

## 1. Preparar o computador

Instala Git, Python 3 e as ferramentas de compilação. Para os testes de geometria local precisas também de um compilador C++17. Usa a instalação oficial do **ESP-IDF 6.0.1**, compatível com a revisão fixada. IDF 5.x não é suportado por esta base. IDF 6.1 pode ser explorado noutro ramo, mas não é o ambiente validado nesta receita.

Fontes oficiais: [início ESP-IDF para ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/get-started/index.html) e [instalação de ferramentas](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/get-started/index.html#installation).

Exemplo macOS/Linux, numa pasta de desenvolvimento tua:

```sh
git clone -b v6.0.1 --recursive https://github.com/espressif/esp-idf.git esp-idf
cd esp-idf
./install.sh esp32s3
. ./export.sh
idf.py --version
```

A saída deve identificar ESP-IDF 6.0.1. Em cada terminal novo, ativa o `export.sh` dessa instalação. Se usares o instalador oficial Windows, abre a consola ESP-IDF por ele criada; adapta os comandos de ativação e os nomes das portas. Esta publicação não verificou fisicamente o fluxo Windows.

## 2. Obter o projeto e preparar as fontes

```sh
git clone https://github.com/Pathink1973/Aimilia-Companion.git
cd Aimilia-Companion
python3 scripts/prepare.py
```

O script:

1. Lê `firmware/manifest.json` e confere os hashes das personalizações.
2. Obtém o upstream público XiaoZhi e seleciona o commit fixado, não a última versão.
3. Copia o overlay, o perfil e o lock de dependências para `work/xiaozhi`.
4. Não lê, liga ou grava nenhuma placa.

Se `work/xiaozhi` já existir, o script pára para preservar alterações. Não usar `git reset --hard` nem apagar essa pasta sem verificar o teu trabalho. A opção `--upstream-local /caminho/de/um/clone` é apenas uma alternativa para preparar a partir de um espelho Git local; a revisão é conferida na mesma.

## 3. Identificar a placa e fazer um backup privado

Liga a Waveshare por USB-C com dados. Identifica a porta que apareceu ao ligar a placa. No macOS costuma ser `/dev/cu.usbmodem...`; em Linux pode ser `/dev/ttyACM0`; em Windows, `COM...`. Não copiar esses nomes sem conferir.

Na consola ESP-IDF, define a **tua** porta numa variável. O exemplo abaixo é Bash; substitui o caminho antes de executar:

```sh
AIMILIA_PORT=/dev/cu.SUBSTITUI_PELA_TUA_PORTA
python -m esptool --chip esp32s3 --port "$AIMILIA_PORT" chip-id
python -m esptool --chip esp32s3 --port "$AIMILIA_PORT" flash-id
python -m esptool --chip esp32s3 --port "$AIMILIA_PORT" get-security-info
```

Confirma ESP32-S3, **32 MB de flash** e segurança compatível. Se não coincidir, pára. Uma falha de deteção pode ser cabo, porta ocupada, alimentação ou necessidade de modo download; segue o procedimento da [Waveshare](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75C) em vez de pressionar botões ao acaso.

Guarda o backup **fora do repositório**, numa pasta privada tua:

```sh
mkdir -p ../aimilia-private-backups
chmod 700 ../aimilia-private-backups
python -m esptool --chip esp32s3 --port "$AIMILIA_PORT" read-flash \
  0x0 0x2000000 ../aimilia-private-backups/antes-da-aimilia-32mb.bin
python - <<'PY'
from pathlib import Path
import hashlib
p=Path('../aimilia-private-backups/antes-da-aimilia-32mb.bin')
assert p.stat().st_size == 32*1024*1024
print('SHA-256:',hashlib.sha256(p.read_bytes()).hexdigest())
PY
```

Guarda o SHA-256 junto do backup e confere uma segunda leitura se houver dúvida de integridade. Um backup pode conter Wi-Fi, tokens, identidade da placa e conversas; nunca o anexes a issues ou ao GitHub. Não gravar se o backup falhar ou se desconheces como recuperar a tua instalação. Um dump de flash encriptada, por si só, não é uma garantia de restauro noutra placa.

## 4. Compilar sem gravar

Volta à raiz `Aimilia-Companion`, com ESP-IDF ativo:

```sh
python3 scripts/build.py
```

O wrapper chama o script canónico do upstream:

```sh
# Executado dentro de work/xiaozhi pelo wrapper; não é necessário repetir.
python3 scripts/build.py waveshare/esp32-s3-touch-amoled-1.75 \
  --config aimilia.config.json \
  --name esp32-s3-touch-amoled-1.75c \
  --language pt-PT --wake-word disabled
```

O perfil usa a placa C, 32 MB flash, AEC no dispositivo, textos pt-PT e palavra de ativação desativada. Inicia-se a conversa pelo botão. Isso não altera a voz cloud para pt-PT. O primeiro build pode demorar e descarregar componentes e modelos; não promete funcionamento offline do processo de compilação.

Os ficheiros de build ficam em `work/xiaozhi/build` e eventuais pacotes em `work/xiaozhi/releases`, ambos fora do repositório público. Confere erros reais antes de prosseguir; não ignorar avisos de tamanho, partições ou placa.

Testes adicionais, sem placa:

```sh
python3 scripts/verify_public.py
python3 scripts/test_firmware.py
cd integrations/cloudflare-mcp
npm ci
npm test
```

## 5. Rever o plano de gravação

Antes de gravar, confirma:

- Tens o backup privado e hash da **mesma placa**.
- A instalação pretendida é standalone, não uma atualização de dual boot.
- O modelo e os controladores correspondem a 1.75C.
- `build/config/sdkconfig.h` identifica a placa C e flash de 32 MB.
- `build/flasher_args.json` e a tabela gerada apontam para os ficheiros acabados de compilar.
- A imagem cabe na partição; os assets correspondem à mesma compilação.
- Nenhum `nvs.bin`, Wi-Fi ou dump de outra pessoa está no plano.

Se usares Codex, pede-lhe que apresente esses ficheiros, tamanhos e offsets e confirme a situação da tua placa. A autorização tem de ser específica para a gravação após essa revisão; não confundir «compilar» com «instalar».

## 6. Gravar apenas na placa standalone aprovada

**Este passo substitui partes do firmware atual. Não o executar numa placa com dual boot a preservar.** A partir da raiz do repositório, com a porta correta e ESP-IDF ativo:

```sh
cd work/xiaozhi
idf.py -p "$AIMILIA_PORT" flash
idf.py -p "$AIMILIA_PORT" monitor
```

O `idf.py flash` usa os argumentos e offsets do build; não os reescrevas a partir de outro tutorial. Não acrescentar `erase-flash`. No monitor, `Ctrl+]` termina a sessão. Não publicar os logs completos sem remover identificadores de rede ou de ativação.

Se existir NVS de outro firmware, pode ser necessária uma limpeza **apenas da partição identificada**, depois de guardar os dados e compreender o impacto. Este guia não automatiza essa operação e não recomenda apagar a flash completa como solução genérica.

## 7. Primeira ligação e verificação física

1. Configura o Wi-Fi segundo o ecrã de arranque do XiaoZhi; usa 2,4 GHz.
2. Ativa a placa na tua consola XiaoZhi com o código apresentado, sem o partilhar.
3. Configura o papel e voz do agente. [Guia do agente →](04-configurar-xiaozhi.md)
4. Faz uma pergunta curta, depois outra mais longa, ficando em silêncio durante a resposta.
5. Verifica voz até ao fim, movimento da boca, skins, gesto do menu, brilho, volume, rede e bateria.
6. Desliga o Mac e verifica a operação normal por Wi-Fi. Só desligar USB depois de concluída a gravação e com alimentação/bateria suficiente.

Regista o resultado na [checklist](07-testes-e-diagnostico.md). Uma compilação bem-sucedida não testa microfone, altifalante, toque, autonomia ou conectividade real.

## Atualizações e recuperação

Mantém uma cópia do teu commit, `sdkconfig` privado e build antes de experimentar alterações. Uma atualização OTA oficial do XiaoZhi pode substituir a personalização; não a aceitar sem verificar compatibilidade e plano de regresso. Para uma placa com Muse/dual boot, consultar [o capítulo próprio](../advanced/dualboot/README.md). Recuperar uma placa exige o mapa e as imagens dela; não usar dumps de outra pessoa.
