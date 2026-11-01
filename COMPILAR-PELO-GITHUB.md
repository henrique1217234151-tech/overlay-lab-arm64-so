# Compilar as bibliotecas .so sem instalar NDK no PC

Este projeto contem um fluxo do GitHub Actions em `.github/workflows/build-arm64.yml`.
Ele faz a compilacao online, usando Android NDK r27c e Dear ImGui v1.91.9.

## Passos

1. Crie um repositorio novo no GitHub.
2. Extraia este ZIP e envie **os arquivos e pastas que estao dentro** da pasta `overlay-lab-dlopen-ci/` para a raiz do repositorio (inclusive a pasta oculta `.github`). Nao envie o ZIP fechado para o repositorio.
3. Acesse **Actions > Gerar bibliotecas Android ARM64 > Run workflow > Run workflow**.
4. Aguarde a execucao. Se estiver verde, abra a execucao e baixe o artefato **overlay-lab-arm64-so**.
5. Dentro do artefato estarao `liboverlay_host.so` e `liboverlay_plugin.so`.

O fluxo e acionado tambem ao enviar arquivos para as branches `main` ou `master`.

**Importante:** o ZIP contem o projeto e a automacao. **Nao contem binarios prontos**.
As duas bibliotecas pertencem ao aplicativo de demonstracao e nao sao injetores de jogos.
