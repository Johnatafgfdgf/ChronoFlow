# ChronoFlow Universal — SDK v0.3 (experimental)

**Objetivo:** uma biblioteca reutilizável que qualquer desenvolvedor possa incorporar, com permissão da licença, ao próprio emulador, port ou motor de jogo. O ChronoFlow **não** é um app que se injeta em outros jogos.

[Guia de integração](docs/INTEGRATION.md) · [API pública](include/chronoflow/chronoflow.h) · [Exemplo C](examples/integration_example.c)

## O que já existe
- Core C++17 com interface binária C99, sem dependências gráficas;
- sugestão de FPS real, FPS apresentado, escala e habilitação condicional de gerador externo;
- relógio de simulação com backlog explícito: não mascara câmera lenta;
- callbacks de integração fornecidos pelo desenvolvedor;
- testes e exemplo com CI em Linux.

## O que ainda NÃO existe
- injeção genérica, driver Vulkan, compositor de imagens, rede neural ou APK;
- frame generation real, motion estimation ou compatibilidade certificada com emuladores;
- aumento de FPS real ou correção universal de jogos em câmera lenta.

## Compilar
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/cf_example
```

Build Android: use NDK + CMake com `-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26`. O SDK gera biblioteca estática para integração no host; não cria um APK sozinho.

## Compatibilidade
Qualquer projeto que possa compilar C++17 e chamar uma API C pode experimentar o núcleo, mas os resultados dependem dos dados e recursos fornecidos pelo host. O suporte gráfico real exigirá adaptadores específicos. Consulte a documentação.

## Política de contribuições
Pull requests com adaptadores, benchmarks reproduzíveis, testes de latência, shaders e otimizações são bem-vindos. Evite prometer melhorias sem medições no dispositivo.

Licença: MIT. Código histórico v0.1/v0.2 será importado separadamente como material experimental, sem afirmar equivalência ao SDK.
