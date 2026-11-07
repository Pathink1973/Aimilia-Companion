# Testes físicos, diagnóstico e recuperação

[← Início](../README.md) · [Estado de validação](08-validacao.md)

## Checklist da tua instalação

Marca apenas o que verificaste na tua placa. Não basta o build terminar.

- [ ] Modelo 1.75C, flash 32 MB, backup privado íntegro e hashes guardados.
- [ ] Arranque sem reboot/panic e ecrã corretamente orientado.
- [ ] Microfones captam uma pergunta compreensível.
- [ ] Resposta curta reproduzida até ao fim.
- [ ] Resposta longa reproduzida até ao fim, sem confundir a própria voz com o utilizador.
- [ ] Boca acompanha a reprodução e pára no fim.
- [ ] Skins Clássica original, Clássica escura e Formas funcionam; voltam após reiniciar.
- [ ] Texto legível em fundos claros e escuros.
- [ ] Menu abre por deslize; volume e luminosidade respondem.
- [ ] Toque/movimento reagem em repouso sem bloquear conversa ou provocar reinício.
- [ ] Percentagem/estado de bateria coerentes com e sem USB.
- [ ] Rede de casa e hotspot compatível funcionam; reconexão após perda de rede.
- [ ] Operação por Wi-Fi sem o Mac ligado, com alimentação suficiente.
- [ ] Se houver MCP, descoberta e contadores confirmam chamadas reais.
- [ ] Agenda/preferências consultáveis após reinício; entrada de teste apagada.
- [ ] Se houver Gmail, só leitura e revogação testada pelo titular.

## Sintomas frequentes

| Sintoma | Investigar primeiro |
| --- | --- |
| Porta USB não aparece | Cabo de dados, outra porta, alimentação e procedimento de download do fornecedor |
| Falha de build | ESP-IDF ativo e versão, revisão base, dependências, espaço; ler primeiro erro, não só o último |
| Ecrã apagado ou toque errado | Variante C, controlador, pinagem e binários do mesmo build |
| Não encontra hotspot | iPhone com «Maximizar compatibilidade», 2,4 GHz, SSID exato e credenciais privadas |
| Wi-Fi funciona mas conversa falha | Acesso ao servidor XiaoZhi, ativação, permissões, DNS, sessão e conectividade móvel |
| Texto continua e áudio pára | Estado TTS, interrupções falsas/AEC e contadores de reprodução; verificar silêncio do utilizador |
| Som baixo a 100% | Nível do áudio de origem, ganho/codec e altifalante; não aumentar ganho arbitrariamente |
| Ferramenta diz «não disponível» | MCP Online, descoberta, verificação da conta e sessão da placa renovada |
| Hora funciona e notícias não | Confirmar chamada MCP real; podem ser ferramentas diferentes |
| Gmail `invalid_grant` | Consentimento expirado/revogado ou configuração OAuth; renovar com o titular |
| Bateria incoerente | Bateria presente/compatível, estado de carga e leitura AXP2101; não estimar autonomia inventada |

## Logs úteis sem expor informação privada

O overlay inclui contadores `VoicePlayback`: pacotes recebidos/rejeitados, áudio descodificado/reproduzido e buffers com sinal. Um contador `audible` não prova que o altifalante emitiu som audível; é um indicador do sinal PCM. O evento «TTS stop» pode ocorrer antes de a fila terminar de tocar.

Anota pergunta, se ficaste em silêncio, duração aproximada da resposta e trecho redigido dos erros. Remove SSID, IP público, códigos de ativação, endpoints com token, emails e conteúdo pessoal antes de partilhar.

Se o aparelho ouvir a sua própria voz como uma segunda pergunta, investigar eco, AEC, volume e modo de escuta. Não atribuir automaticamente o problema a «falta de inteligência» ou a animação pesada sem medição.

## Recuperação

1. Não repetir flash completa por tentativa.
2. Regista o erro e conserva a versão que funcionava.
3. Confirma cabo, alimentação e modo de arranque.
4. Inspeciona tabela/offsets antes de propor qualquer escrita.
5. Usa o backup da mesma placa e o plano de restauro compatível com o seu estado de segurança. Não publicar o dump.

Em dual boot, o slot Aimilia do layout de referência é `0x900000` com 3 MB, mas **só é válido se a tabela real da tua placa coincidir**. Não é o offset do perfil standalone nem um comando universal de recuperação.
