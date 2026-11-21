# Integrando ChronoFlow ao seu emulador ou port

## Objetivo
A biblioteca é independente de Winlator, GameHub, Unity, Godot, DXVK, Vulkan e Android. Um autor de emulador ou port pode compilar o núcleo e criar o adaptador que conecta os dados e funções do host.

## Contrato mínimo
1. Inclua `chronoflow/chronoflow.h` no seu projeto. Compile o core com CMake (C++17); a API pública é C e o exemplo compila como C99.
2. Meça FPS efetivo, custos CPU e GPU por frame, filas e limite térmico. Não use estimativas fictícias em produção.
3. Preencha `cf_telemetry` com valores medidos e os recursos que seu host oferece.
4. Chame `cf_choose_policy`. O resultado é uma sugestão, não uma garantia de desempenho.
5. Implemente `cf_host_v1` para aplicar os alvos de taxa de renderização, escala e ativação da sua própria tecnologia de framegen.
6. Opcional: chame `cf_timeline_advance` se o emulador controlar a simulação; execute exatamente os ticks pedidos e monitore a dívida. Nunca use em simuladores cujo relógio é externo sem entender a semântica de tempo.
7. Verifique qualidade de imagem, latência, estabilidade e desempenho com captura GPU. Se framegen piorar FPS real ou latência, desligue.

## Onde integrar
- Emuladores de PC no Android (Wine/DXVK/VKD3D): adaptador ao código do renderizador ou frontend, com permissão do projeto; nunca presumir acesso à lógica do jogo Windows.
- Emuladores de console: integrar ao compositor ou GPU virtualizada; a temporização da CPU emulada deve permanecer autoritativa.
- Ports Android: idealmente integrar à engine para ter input, camera, depth e motion vectors.
- OpenGL ES: criar adaptador específico; a biblioteca não contém interceptação OpenGL.
- Vulkan: criar módulo de apresentação e shader; **ainda não incluído nesta versão**.
- GameHub: somente por integração oferecida pelo aplicativo ou por desenvolvimento sobre código licenciado.

## Limites e segurança
- O SDK não injeta código em terceiros, não modifica jogos fechados e não contorna anti-cheat.
- `has_motion_vectors`, `has_depth` e `can_generate_frames` devem ser verdadeiros apenas se o host implementar os recursos.
- Nem SDK nem framegen corrigem gargalos de CPU, problemas de sincronização internos ou lentidão na emulação sem mudanças no host.
- Se a CPU não sustenta os ticks de simulação, o core sinaliza o gargalo. Não afirma corrigir a velocidade do jogo.
- Alvos de FPS não são FPS alcançados. Esta versão NÃO faz renderização real ou geração de imagens.

## Plano de implementação
1. C ABI + testes Linux/Android NDK.
2. Adaptador de referência para um renderizador Vulkan open source com acesso a profundidade e vetores.
3. Frame pacing e métricas de latência.
4. Reprojeção GPU com máscara de confiabilidade, fallback em oclusões e UI separada.
5. Benchmark real em emuladores/ports (qualidade, temperatura, FPS real, simulação em tempo real).
