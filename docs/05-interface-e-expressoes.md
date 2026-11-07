# Interface, expressões e personalidade visual

[← Início](../README.md)

## Três aparências

- **Clássica original:** restaura as superfícies e o aspeto clássico do ecrã.
- **Clássica escura:** olhos e boca ciano num fundo escuro.
- **Formas:** desenho colorido, escolha de forma, fundo e cor no menu. O desenho é procedural, não um pacote de vídeos.

Na secção Aparência das definições, alterna e verifica que a escolha fica guardada após reiniciar. O texto de cabeçalho adapta o contraste aos fundos claros e escuros.

## 18 geometrias canónicas

| ID | Nome no código | Leitura visual |
| --- | --- | --- |
| 01 | normal | Neutra |
| 02 | angry | Zangada |
| 03 | glee | Radiante |
| 04 | happy | Contente |
| 05 | sad | Triste |
| 06 | worried | Preocupada |
| 07 | focused | Concentrada |
| 08 | annoyed | Irritada |
| 09 | surprised | Surpreendida |
| 10 | skeptic | Cética |
| 11 | frustrated | Frustrada |
| 12 | unimpressed | Indiferente |
| 13 | sleepy | Ensonada |
| 14 | suspicious | Desconfiada |
| 15 | squint | Olhos semicerrados |
| 16 | furious | Furiosa |
| 17 | scared | Assustada |
| 18 | awe | Maravilhada |

Definidas em `expressive_eyes.h`; emoções de servidor adicionais podem ser mapeadas para estas poses e detalhes. «Embarrassed» desencadeia timidez e rubor; não significa uma 19.ª geometria canónica independente. Não se afirma copiar integralmente as animações de outro produto.

O movimento inclui piscar, olhar, interpolação, sobrancelhas, lágrimas ou rubor quando aplicável. A boca acompanha a energia do áudio, não fonemas ou palavras exatas. O contexto local é baseado em regras limitadas; não avalia emoções humanas.

## Gestos e controlos

- Deslize **horizontal** abre as definições na placa. O simulador do website usa scroll vertical apenas para demonstração.
- Volume, luminosidade, redes, aparência e bateria ficam no menu.
- Toque e agitação podem provocar reações quando o estado da aplicação permite. Há limitação de frequência para não interromper a conversa.
- Na edição pública, os sons locais de reação são sinais curtos não vocais, criados para este pacote. Não são os excertos de vozes do sistema usados no protótipo privado.
- Botão curto controla a conversa segundo o estado do XiaoZhi. No perfil standalone, duplo clique segue o comportamento AEC do código; **não abre um menu Muse inexistente**.
- No dual boot de referência, duplo clique regressa ao seletor, mas só com a tabela e guards correspondentes.

## Bateria

A leitura vem do PMIC AXP2101. Percentagem e estado de carga não medem a saúde eletroquímica nem garantem horas restantes. Sem bateria compatível ou com falha de leitura, tratar o estado como desconhecido, não inventar uma percentagem. Testar ligado e desligado do USB. Não alterar tensões de carga por tentativa.

## Regras para alterar gráficos

Preservar tarefas de áudio, evitar alocações grandes repetidas, respeitar PSRAM, não bloquear o event loop e medir duração de frame. O ecrã tem muitos píxeis, mas o dispositivo não é um computador com GPU 3D. A apresentação Three.js do site não corre no ESP32.
