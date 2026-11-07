# Arranque duplo — referência avançada, não instalador universal

[← Início](../../README.md) · [Instalação standalone](../../docs/03-instalacao.md)

O protótipo do autor mantém Muse e Aimilia/XiaoZhi na mesma placa, com um seletor. Aqui ficam as fontes do seletor e guardas de integração para estudo e reprodução técnica **depois de auditar a tua placa**. Não se distribuem binários Muse, chaves, credenciais, NVS ou backups.

## O que existe nesta pasta

- `selector/`: aplicação ESP-IDF do menu com estilo visual Aimilia, CMake, dependências e defaults genéricos.
- `components/dualboot_guard/`: wrappers de arranque, NVS e operações OTA que isolam o estado XiaoZhi e limitam escritas de atualização.
- `partitions.csv`: mapa de referência de 32 MB. Não é um mapa universal do firmware Waveshare/Muse.

| Partição de referência | Offset | Tamanho | Função |
| --- | --- | --- | --- |
| nvs | 0x11000 | 0xC000 | Estado existente do Muse |
| ota_0 / ota_1 | 0x20000 / 0x420000 | 4 MB cada | Apps Muse existentes |
| prod_data / prod_bak | 0x820000 / 0x821000 | 4 KB cada | Dados de produção existentes |
| xiao_nvs | 0x830000 | 0xB0000 | Estado XiaoZhi separado |
| nvsfactory | 0x8E0000 | 0x20000 | Dados reservados |
| xiaozhi | 0x900000 | 3 MB | Aimilia |
| selector | 0xC00000 | 1 MB | Menu de arranque |
| assets | 0xD00000 | 3 MB | Assets XiaoZhi |

A tabela está em `0x10000`. O guia standalone usa outra tabela e **não pode ser usado para atualizar este layout**. Validar também bootloader, OTA data, flags e políticas de assinatura existentes.

## Plano para um maker experiente / Codex

1. Identificar modelo, segurança e flash da própria placa, sem escrita.
2. Guardar e verificar backup completo e partições. Não reutilizar o backup do autor.
3. Comparar a tabela real com esta referência; se diferir, redesenhar o plano em vez de forçar offsets.
4. Compilar Aimilia com a mesma revisão e overlay, perfil de 32 MB, tabela dual e `CONFIG_DUALBOOT_XIAO_NVS=y`, incluindo `components/dualboot_guard` em `EXTRA_COMPONENT_DIRS` e em `COMPONENTS`.
5. Compilar o seletor com a mesma tabela. Paths de configuração devem ser relativos ou resolvidos na tua máquina; não reutilizar caminhos absolutos de outra instalação.
6. Confirmar por símbolos que os wrappers foram ligados e que cada aplicação/asset cabe no seu slot.
7. Verificar a política de assinatura do firmware Muse na própria instalação. Gerar chaves de desenvolvimento próprias quando aplicável; não copiar chaves do autor nem mudar eFuses para contornar validação.
8. Apresentar plano de offsets e riscos. Só gravar depois de autorização específica e de a recuperação estar preparada.
9. Testar ida e volta pelo menu, persistência de ambas as apps, áudio e atualização sem sobrescrever slots indevidos.

Não existe neste repositório um `install.py` que execute estes passos automaticamente. Essa decisão é intencional: um script ligado ao mapa privado do autor seria uma receita insegura e não reproduzível para outras placas. O caminho principal público é standalone e está documentado separadamente.
